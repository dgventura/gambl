
// Game_Music_Box 0.5.2. http://www.slack.net/~ant/game-music-box

#include "File_Archive.h"

#include "Zip_Extractor.h"
#include "Gzip_Reader.h"

// to do: extract common index handling code

/* Copyright (C) 2005 Shay Green. This module is free software; you
can redistribute it and/or modify it under the terms of the GNU Lesser
General Public License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version. This
library is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for
more details. You should have received a copy of the GNU Lesser General
Public License along with this module; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA */

#include "source_begin.h"

File_Archive::File_Archive()
{
	std::memset( &info_, 0, sizeof info_ );
}

struct Managed_Mem_Reader : Mem_File_Reader
{
	char* begin;
	Managed_Mem_Reader( long s ) : Mem_File_Reader( (begin = new char [s]), s ) {
	}
	~Managed_Mem_Reader() {
		delete [] begin;
	}
};

Data_Reader* File_Archive::extract()
{
	unique_ptr<Managed_Mem_Reader> reader( new Managed_Mem_Reader( info_.size ) );
	extract( reader->begin, info_.size );
	return reader.release();
}

void File_Archive::extract( void* p, long s )
{
	unique_ptr<Data_Reader> in( extract() );
	in->read( p, s );
}

void File_Archive::uncache()
{
}

// File

struct Single_File_Archive : File_Archive
{
	unique_ptr<Gzip_Reader> file;
	GaMBLFileHandle path;
	char filename [256 + 8];
	
	Single_File_Archive( const GaMBLFileHandle& path_, const char* fn ) : path( path_ )
	{
		info_.is_file = true;
		info_.name = filename;
		std::strcpy( filename, fn );
	}
	
	bool seek( int i, bool might_extract )
	{
		if ( i != 0 )
			return false;
		
		if ( !file && might_extract ) {
			file.reset( new Gzip_Reader( path ) );
			info_.size = file->remain();
		}
		return true;
	}
	
	Data_Reader* extract() {
		return file.release();
	}
	
	void uncache() {
		file.reset();
	}
};

File_Archive* open_file_archive( const GaMBLFileHandle& path, const char* filename ) {
	return new Single_File_Archive( path, filename );
}

// ZIP

struct Zip_Archive : File_Archive
{
	Mac_File_Reader reader;
	Zip_Extractor arc;
	int index;
	
	Zip_Archive( const GaMBLFileHandle& path ) : reader( path )
	{
		index = -1;
		if ( !arc.open( &reader ) )
			throw_error( "Not a ZIP archive" );
	}
	
	bool seek( int new_index, bool might_extract )
	{
		if ( new_index < index )
			rewind();
		
		while ( index < new_index )
		{
			index++;
			if ( !arc.next() )
				return false;
		}
		
		info_.is_file = arc.info().is_file;
		info_.name = arc.info().name;
		info_.size = arc.info().size;
		
		return true;
	}
	
	void extract( void* p, long s )
	{
		arc.extract( p, s );
		index++;
		arc.next();
	}
	
	//Data_Reader* extract() {
	//  return arc.extract();
	//}
	
	void rewind() {
		arc.rewind();
		index = -1;
	}
	
	void uncache() {
		// to do: implement?
	}
};

File_Archive* open_zip_archive( const GaMBLFileHandle& path ) {
	return new Zip_Archive( path );
}

// RAR

#if UNRAR_AVAILABLE

#include "Rar_Extractor.h"

void rar_out_of_memory()
{
	throw std::bad_alloc();
}

struct Rar_Archive : File_Archive
{
	Mac_File_Reader reader;
	Rar_Extractor arc;
	int index;
	bool scan_only;
	bool is_open;
	
	void rewind();
	
	Rar_Archive( const GaMBLFileHandle& path_ ) : reader( path_ )
	{
		is_open = false;
		rewind();
	}
	
	bool seek( int new_index, bool might_extract )
	{
		if ( !is_open || (might_extract && scan_only) || new_index < index )
			rewind();
		scan_only = !might_extract;
		if ( scan_only )
			arc.scan_only();
		
		while ( index < new_index )
		{
			index++;
			if ( throw_unless( arc.next(), end_of_rar ) )
				return false;
		}
		
		info_.is_file = arc.info().is_file;
		info_.name = arc.info().name;
		info_.size = arc.info().size;
		
		return true;
	}
	
	void extract( void* p, long s)
	{
		require( info_.is_file );
		Mem_Writer out( p, s );
		throw_error( arc.extract( out ) );
		index++;
		if ( throw_unless( arc.next(), end_of_rar ) )
			uncache();
	}
	
	void uncache();
};

void Rar_Archive::uncache()
{
	if ( is_open ) {
		is_open = false;
		arc.close();
	}
}

void Rar_Archive::rewind()
{
	uncache();
	reader.seek( 0 );
	if ( throw_unless( arc.open( &reader ), not_rar_file ) )
		throw_error( "Not an RAR archive" );
	is_open = true;
	index = -1;
	scan_only = false;
}
	
File_Archive* open_rar_archive( const GaMBLFileHandle& path ) {
	return new Rar_Archive( path );
}

#else

struct Empty_Archive : File_Archive
{
	bool seek( int, bool ) {
		return false;
	}
	
	Data_Reader* extract() {
		return NULL;
	}
};

File_Archive* open_rar_archive( const GaMBLFileHandle& ) {
	return new Empty_Archive;
}

#endif


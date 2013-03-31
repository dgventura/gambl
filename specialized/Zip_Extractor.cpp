
// Zip_Extractor 0.1.0

#include "Zip_Extractor.h"

#include <assert.h>
#include <string.h>
#include "zlib/zlib.h"

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

typedef unsigned char byte;

struct header_t {
	char type [4];
	byte vers [2];
	byte flags [2];
	byte method [2];
	byte time [2];
	byte date [2];
	byte crc [4];
	byte raw_size [4];
	byte size [4];
	byte filename_len [2];
	byte extra_len [2];
	//char filename [filename_len];
	//char extra [extra_len];
};

struct Zip_Extractor::entry_t {
	char type [4];
	byte made_by [2];
	byte vers [2];
	byte flags [2];
	byte method [2];
	byte time [2];
	byte date [2];
	byte crc [4];
	byte raw_size [4];
	byte size [4];
	byte filename_len [2];
	byte extra_len [2];
	byte comment_len [2];
	byte disk [2];
	byte int_attrib [2];
	byte ext_attrib [4];
	byte file_offset [4];
	//char filename [filename_len];
	//char extra [extra_len];
	//char comment [comment_len];
};

struct end_entry_t {
	char type [4];
	byte disk [2];
	byte first_disk [2];
	byte disk_entry_count [2];
	byte entry_count [2];
	byte dir_size [4];
	byte dir_offset [4];
	byte comment_len [2];
	//char comment [comment_len];
};

Zip_Extractor::Zip_Extractor()
{
	in = NULL;
	catalog = NULL;
	
	assert( sizeof (header_t) == 30 );
	assert( sizeof (entry_t) == 46 );
	assert( sizeof (end_entry_t) == 22 );
}

Zip_Extractor::~Zip_Extractor()
{
	close();
}

class zip_mem_deleter {
	byte* mem;
public:
	zip_mem_deleter( byte* p ) : mem( p ) { }
	~zip_mem_deleter() { delete [] mem; }
};

static void throw_zlib_error( int code )
{
	if ( code )
		RAISE_ERROR( zError( code ) );
}

// read 4-byte little endian value
inline unsigned get32( const unsigned char p [4] ) {
	return p [3] * 0x1000000u + p [2] * 0x10000u + p [1] * 0x100u + p [0];
}

// read 2-byte little endian value
inline unsigned get16( const unsigned char p [2] ) {
	return p [1] * 0x100u + p [0];
}

bool Zip_Extractor::open( File_Reader* file )
{
	close();
	
	// try reading end of catalog entry with no comment
	long file_size = file->size();
	end_entry_t entry;
	if ( file_size < sizeof entry )
		return false;
	
	long end_offset = file_size - sizeof entry;
	file->seek( end_offset );
	file->read( &entry, sizeof entry );
	
	int last_int16 = 0;
	
	if ( 0 != strncmp( entry.type, "PK\5\6", 4 ) )
	{
		// end entry not found; scan backwards in case there is comment
		
		// read last few kbytes of file
		end_offset = file_size - 8 * 1024L; // search 8K
		if ( end_offset < 0 )
			end_offset = 0;
		int buf_size = file_size - end_offset;
		byte* buf = new byte [buf_size];
		if ( !buf )
			RAISE_ERROR( "Out of memory" );
		{
			zip_mem_deleter deleter( buf );
			
			file->seek( end_offset );
			file->read( buf, buf_size );
			last_int16 = buf [buf_size - 2] * 0x100L + buf [buf_size - 1];
			int i = buf_size - sizeof entry;
			while ( 0 != memcmp( buf + i, "PK\5\6", 4 ) )
			{
				if ( --i < 0 )
				{
					#ifndef NDEBUG
						file->seek( 0 );
						char buf [2];
						file->read( buf, sizeof buf );
						// didn't find header; be sure this isn't a zip file
						assert( buf [0] != 'P' || buf [1] != 'K' );
					#endif
					return false;
				}
			}
			end_offset += i;
		}
		
		file->seek( end_offset );
		file->read( &entry, sizeof entry );
	}
	
	// verify that we didn't find something that looks like a header in comment.
	// ZipIt for Mac OS puts data after the end, including a big-endian two byte
	// value at the end holding the size of the extra data.
	long zip_end = end_offset + sizeof entry + get16( entry.comment_len );
	if ( zip_end != file_size && zip_end + last_int16 != file_size ) {
		assert( false );
		return false;
	}
	
	// read catalog into memory
	long dir_offset = get32( entry.dir_offset );
	assert( dir_offset < file_size );
	file->seek( dir_offset );
	long catalog_size = end_offset + sizeof entry - dir_offset;
	catalog = new char [catalog_size];
	if ( !catalog )
		RAISE_ERROR( "Out of memory" );
	file->read( catalog, catalog_size );
	
	// first entry in catalog should be a file
	if ( 0 != strncmp( catalog, "PK\1\2", 4 ) ) {
		assert( false );
		return false;
	}
	
	in = file;
	offset = 0;
	advance_entry = false;
	memset( &info_, 0, sizeof info_ );
	
	return true;
}

void Zip_Extractor::close()
{
	delete [] catalog;
	catalog = NULL;
	in = NULL;
}

bool Zip_Extractor::next()
{
	if ( advance_entry )
	{
		// find next entry
		const entry_t& e = *(entry_t*) (catalog + offset);
		offset += sizeof e + get16( e.filename_len ) + get16( e.extra_len ) +
				get16( e.comment_len );
	}
	advance_entry = true;
	
	const entry_t& e = *(entry_t*) (catalog + offset);
	if ( 0 != strncmp( e.type, "PK\1\2", 4 ) ) {
		assert( 0 == strncmp( e.type, "PK\5\6", 4 ) );
		return false;
	}
	
	info_.size = get32( e.size );
	info_.is_file = (info_.size > 0);
	int len = get16( e.filename_len );
	if ( len >= sizeof info_.name )
		len = sizeof info_.name - 1;
	info_.name [len] = 0;
	memcpy( info_.name, catalog + offset + sizeof e, len );
	
	return true;
}

void Zip_Extractor::extract( void* out, long out_size )
{
	assert( out_size <= info_.size );
	assert( info_.is_file );
	
	const entry_t& e = *(entry_t*) (catalog + offset);
	if ( get16( e.vers ) > 20 )
		RAISE_ERROR( "Zip file uses unsupported compression" );
	
	// seek to file data
	long file_offset = get32( e.file_offset );
	header_t header;
	in->seek( file_offset );
	in->read( &header, sizeof header );
	assert( 0 == strncmp( header.type, "PK\3\4", 4 ) );
	in->skip( get16( header.filename_len ) + get16( header.extra_len ) );
	long raw_size = get32( e.raw_size );
	
	int method = get16( e.method );
	if ( method == Z_DEFLATED )
	{
		static z_stream default_stream = { };
		z_stream z = default_stream;
		throw_zlib_error( inflateInit2( &z, -MAX_WBITS ) );
		
		// to do: remove dummy byte when new version of zlib is upgraded to
		// to do: verify that new zlib doesn't need dummy byte
		
		// read raw data
		//byte* data = new byte [raw_size + 1];
		byte* data = new byte [raw_size + 1];
		if ( !data )
			RAISE_ERROR( "Out of memory" );
		//data [raw_size] = 0;
		zip_mem_deleter deleter( data );
		in->read( data, raw_size );
		
		// inflate
		z.next_in = data;
		//z.avail_in = raw_size + 1; // include dummy byte needed
		z.avail_in = raw_size;
		z.next_out = (byte*) out;
		z.avail_out = out_size;
		int result = inflate( &z, out_size == info_.size ? Z_FINISH : Z_SYNC_FLUSH );
		inflateEnd( &z );
		if ( result != Z_STREAM_END )
			throw_zlib_error( result );
		assert( z.avail_out == 0 );
		
		// check crc (if all file data was read)
		if ( out_size == info_.size )
		{
			unsigned long crc = crc32( 0, NULL, 0 );
			crc = crc32( crc, (Bytef*) out, out_size );
			if ( crc != get32( e.crc ) )
				RAISE_ERROR( "CRC error in zip file" );
		}
	}
	else if ( method == 0 )
	{
		// not compressed
		assert( raw_size == info_.size );
		in->read( out, out_size );
	}
	else {
		RAISE_ERROR( "Unsupported compression method in zip file" );
	}
}


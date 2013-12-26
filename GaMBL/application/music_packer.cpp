
// Game music compression/decompression

// Game_Music_Box 0.5.2. http://www.slack.net/~ant/game-music-box

#include "music_actions.h"

#include "FileUtilities.h"
#include "Spc_Packer.h"
#include "Gzip_Reader.h"
#include "unpack_spc.h"

#include "zlib/zlib.h"
#include "zlib/zutil.h"

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

static OSType is_packable( const Cat_Info& info )
{
	if ( info.is_dir() || info.is_alias() )
		return false;
	
	if ( info.finfo().fileCreator != gmb_creator )
		return false;
	
	return is_music_type( info.finfo().fileType );
}

class Gzip_Writer : public Spc_Writer {
	Mac_File file;
public:
	Gzip_Writer( const GaMBLFileHandle& );
	~Gzip_Writer();
	
	// Can only accept one write
	const char* write( const void*, long );
};

Gzip_Writer::Gzip_Writer( const GaMBLFileHandle& path ) : file( path ) {
}

Gzip_Writer::~Gzip_Writer() {
}

const char* Gzip_Writer::write( const void* p, long s )
{
	require( file.size() == 0 );
	write_gzip_file( p, s, file );
	return NULL;
}

static int DeprecatedFSOpenForkExists( const GaMBLFileHandle& path, int perm, const HFSUniStr255* fork_name )
{
	FSIORefNum ref = 0;
	OSErr err = DeprecatedFSOpenFork( &path, fork_name ? fork_name->length : 0,
			fork_name ? fork_name->unicode : NULL, perm, &ref );
	if ( err == eofErr )
		return 0;
	throw_file_error( err, path );
	assert( ref );
	return ref;
}

static void copy_res_fork( const GaMBLFileHandle& in_path, const GaMBLFileHandle& out_path )
{
	static HFSUniStr255 res_fork_name;
	if ( !res_fork_name.length )
		debug_if_error( DeprecatedFSGetResourceForkName( &res_fork_name ) );
	
	SInt16 in_ref = DeprecatedFSOpenForkExists( in_path, fsRdPerm, &res_fork_name );
	if ( !in_ref )
		return;
	
	Mac_File_Reader in( in_ref );
	long remain = in.size();
	if ( remain == 0 )
		return;
	
	throw_file_error( DeprecatedFSCreateFork( &out_path, res_fork_name.length, res_fork_name.unicode ),
			out_path );
	Mac_File out( DeprecatedFSOpenForkExists( out_path, fsRdWrPerm, &res_fork_name ) );
	
	runtime_array<char> buf( 16 * 1024L );
	
	while ( remain )
	{
		long n = buf.size();
		if ( n > remain )
			n = remain;
		
		remain -= n;
		in.read( buf.begin(), n );
		out.write( buf.begin(), n );
	}
}

const int restore_file_info_flags =
		kFSCatInfoFinderInfo | kFSCatInfoCreateDate | kFSCatInfoContentMod;
static void restore_file_info( const GaMBLFileHandle& path, const Cat_Info& new_info )
{
	// copy file info from original
	Cat_Info info;
	info.read( path, restore_file_info_flags );
			
	info.finfo().finderFlags &= ~kColor;
	info.finfo().finderFlags |= new_info.finfo().finderFlags & kColor;
	
	info.finfo().fileType    = new_info.finfo().fileType;
	info.finfo().fileCreator = new_info.finfo().fileCreator;
	
	info.createDate     = new_info.createDate;
	info.contentModDate = new_info.contentModDate;
	
	info.write( path, restore_file_info_flags );
}

static void pack_spc_set( const GaMBLFileHandle& dir, Action_Hooks* hook = NULL )
{
	unique_ptr<Spc_Packer> packer( new Spc_Packer );
	
	HFSUniStr255 shared_filename;
	FSGetCatalogInfoChk( dir, 0, NULL, &shared_filename );
	if ( shared_filename.length > max_filename - 4 )
		shared_filename.length = max_filename - 4;
	shared_filename.unicode [shared_filename.length++] = '.';
	shared_filename.unicode [shared_filename.length++] = 'd';
	shared_filename.unicode [shared_filename.length++] = 'a';
	shared_filename.unicode [shared_filename.length++] = 't';
	
	{
		char str [256];
		filename_to_str( shared_filename, str );
		throw_error( packer->init( str ) );
	}
	
	int even_odd = 0;
	
	// Read files, then pack them
	for ( int phase = 2; phase--; )
	{
		for ( Dir_Iterator iter( dir ); iter.next( restore_file_info_flags ); )
		{
			even_odd ^= 1;
			
			OSType type = is_packable( iter );
			if ( type != spc_type && type != spcp_type )
				continue;
			
			if ( phase )
			{
				runtime_array<char> data;
				read_packed_spc( iter.ref(), data );
				throw_error( packer->add_spc( data.begin(), data.size() ) );
				
				if ( hook && !hook->advance( iter.ref(), even_odd ) )
					return;
			}
			else
			{
				if ( hook )
					hook->advance( iter.ref(), even_odd );
				
				// to do: write shared file if an error occurs before writing files,
				// to prevent loss of data.
				File_Swapper swapper( iter.ref() );
				{
					Gzip_Writer writer( swapper );
					throw_error( packer->pack_spc( writer ) );
				}
				copy_res_fork( iter.ref(), swapper );
				swapper.swap();
				
				iter.finfo().fileType = spcp_type;
				restore_file_info( iter.ref(), iter );
			}
		}
	}
	
	// Write shared file
	GaMBLFileHandle path;
	if ( DeprecatedFSMakeFSRefExists( dir, shared_filename, &path ) )
		FSDeleteObject( &path );
	Gzip_Writer writer( create_file( dir, shared_filename,
			shared_type, gmb_creator ) );
	throw_error( packer->write_shared( writer ) );
}

static void write_file( const GaMBLFileHandle& path, const void* begin, long size, OSType new_type = 0 )
{
	Cat_Info info;
	info.read( path, restore_file_info_flags );
	
	File_Swapper swapper( path );
	{
		Mac_File out( swapper );
		out.write( begin, size );
	}
	copy_res_fork( path, swapper );
	swapper.swap();
	
	if ( new_type )
		info.finfo().fileType = new_type;
	
	restore_file_info( path, info );
}

static void unpack_spc( const GaMBLFileHandle& path )
{
	runtime_array<char> data;
	if ( !read_packed_spc( path, data ) )
		return; // already unpacked
	
	write_file( path, data.begin(), data.size(), spc_type );
}

static void gzip_file( const GaMBLFileHandle& path )
{
	runtime_array<char> data;
	{
		Mac_File_Reader in( path );
		
		// see if already gzipped
		unsigned char header [2];
		in.read( header, sizeof header );
		if ( header [0] == 0x1F && header [1] == 0x8B )
			return;
		
		// read data
		// to do: optimize this seek out by copying already-read header bytes?
		in.seek( 0 );
		data.resize( in.size() );
		in.read( data.begin(), in.size() );
	}
	
	Cat_Info info;
	info.read( path, restore_file_info_flags );
	
	File_Swapper swapper( path );
	{
		Gzip_Writer out( swapper );
		out.write( data.begin(), data.size() );
	}
	copy_res_fork( path, swapper );
	swapper.swap();
	
	restore_file_info( path, info );
}

static void ungzip_file( const GaMBLFileHandle& path )
{
	runtime_array<char> data;
	{
		Gzip_Reader in( path );
		if ( !in.is_deflated() )
			return; // not gzipped
		data.resize( in.remain() );
		in.read( data.begin(), data.size() );
	}
	
	write_file( path, data.begin(), data.size() );
}

// compress_music

static void compress_item( const Cat_Info& info, Action_Hooks& hooks, bool pack_spcs )
{
	if ( info.is_dir() )
	{
		
		// to do: re-enable when re-adding packed SPC support
		/*
         
         int spc_count = 0;
		
		if ( pack_spcs && !hooks.is_scanning() )
		{
			// find eligible SPC files
			for ( Dir_Iterator iter( info.ref() ); iter.next(); )
			{
				OSType type = is_packable( iter );
				if ( type )
				{
					if ( type == spc_type ) {
						spc_count++;
					}
					else {
						spc_count = 0;
						break;
					}
				}
			}
		}
		
		if ( spc_count > 1 )
			pack_spc_set( info.ref(), &hooks );
		*/
		
		for ( Dir_Iterator iter( info.ref() ); iter.next(); )
		{
			compress_item( iter, hooks, pack_spcs );
			if ( hooks.give_time() )
				return;
		}
	}
	else
	{
		OSType type = is_packable( info );
		if ( type && hooks.advance( info.ref() ) && type != spcp_type )
		{
			try {
				gzip_file( info.ref() );
			}
			catch ( ... ) {
				hooks.log_exception( info.ref() );
			}
		}
	}
}

void compress_music( const GaMBLFileHandle& path, Action_Hooks& hooks, bool pack_spcs )
{
	Cat_Info info;
	info.read( path, kFSCatInfoNodeFlags | kFSCatInfoFinderInfo );
	compress_item( info, hooks, pack_spcs );
}

// expand_music

static void expand_item( const Cat_Info& info, Action_Hooks& hooks )
{
	if ( info.is_dir() )
	{
		GaMBLFileHandle shared_path;
		bool shared_found = false;
		
		for ( Dir_Iterator iter( info.ref() ); iter.next(); )
		{
			// note path of shared data file for deletion later
			if ( !iter.is_dir() && !iter.is_alias() &&
					iter.finfo().fileType == shared_type &&
					iter.finfo().fileCreator == gmb_creator )
			{
				// to do: delete all shared files, not just the last
				shared_path = iter.ref();
				shared_found = true;
			}
			
			expand_item( iter, hooks );
			
			if ( hooks.give_time() )
				return;
		}
		
		if ( shared_found && !hooks.is_scanning() )
			FSDeleteObject( &shared_path );
	}
	else
	{
		OSType type = is_packable( info );
		if ( type && hooks.advance( info.ref() ) )
		{
			try
			{
				if ( type == spcp_type )
					unpack_spc( info.ref() );
				else
					ungzip_file( info.ref() );
			}
			catch ( ... )
			{
				hooks.log_exception( info.ref() );
			}
		}
	}
}

void expand_music( const GaMBLFileHandle& path, Action_Hooks& hooks )
{
	Cat_Info info;
	info.read( path, kFSCatInfoNodeFlags | kFSCatInfoFinderInfo );
	expand_item( info, hooks );
}


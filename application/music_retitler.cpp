
// Game music retitling

// Game_Music_Box 0.5.2. http://www.slack.net/~ant/game-music-box

#include <ctype.h>
#include "music_actions.h"

#include "Multi_Buffer.h"
#include "Gzip_Reader.h"
#include "Music_Album.h"
#include "pod_vector.h"
#include "Music_Emu.h"
#include "file_util.h"

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

struct spc_header_t {
	char tag [35];
	byte format;
	byte version;
	byte pc [2];
	byte a, x, y, psw, sp;
	byte unused [2];
	char song [32];
	char game [32];
	char dumper [16];
	char comment [32];
	byte date [11];
	char len_secs [3];
	byte fade_msec [5];
	char author [32];
	byte mute_mask;
	byte emulator;
	byte unused2 [45];
};

static void extract_spc_str( const char* in, int in_size, char* out )
{
	std::memcpy( out, in, in_size );
	out [in_size] = 0;
	
	// skip initial whitespace and junk
	char* p = out;
	while ( *p && *p <= ' ' )
		++p;
	
	std::memmove( out, p, in_size + 1 - (p - out) );
	
	// remove trailing whitespace and junk
	p = str_end( out );
	while ( p != out && p [-1] <= ' ' )
		p--;
	*p = 0;
}

static bool read_spc_header( const FSRef& path, spc_header_t* out )
{
	Gzip_Reader file( path );
	file.read( out, sizeof *out );
	return (0 == std::strncmp( out->tag, "SNES-SPC700 Sound File Data", 27 ));
}

static bool extract_track_number( const char* in, char* out )
{
	const char* p = in;
	
	// disc number/letter "[0-9a-z]-" (optional)
	if ( (isdigit( p [0] ) || islower( p [0] )) && p [1] == '-' )
		p += 2;
	
	// 1-3 letter prefix "[a-z][a-z][a-z]" (optional)
	for ( int n = 3; n--; )
		if ( islower( *p ) )
			++p;
	
	// track number "[0-9][0-9][0-9]?" (optional)
	if ( !isdigit( *p ) )
		return false;
	if ( isdigit( *p ) )
		++p;
	if ( isdigit( *p ) )
		++p;
	
	// suffix "[a-z]" (optional)
	if ( islower( *p ) )
		++p;
	
	// track must be followed by nothing, space, or '.'
	if ( !*p || isspace( *p ) || *p == '.' )
	{
		int len = p - in;
		std::memcpy( out, in, len );
		out [len] = 0;
	}
	
	return true;
}

static void get_spc_info( const FSRef& path, char* name, char* game )
{
	// to do: use extended SPC info?
	
	spc_header_t h;
	if ( !read_spc_header( path, &h ) )
		return;
	
	extract_spc_str( h.game, sizeof h.game, game );
	
	char title [64];
	extract_spc_str( h.song, sizeof h.song, title );
	title [sizeof title - 1] = 0;
	
	if ( !*title )
		return;
	
	char old [256];
	filename_without_extension( path, old );
	
	name [0] = 0;
	
	if ( !extract_track_number( old, name ) )
	{
		const char* last = std::strrchr( old, '-' );
		if ( last )
		{
			const char* p = last;
			while ( true )
			{
				if ( p == old ) {
					extract_track_number( last + 1, name );
					break;
				}
				if ( *--p == '-' )
				{
					extract_track_number( p + 1, name );
					break;
				}
			}
		}
	}
	
	if ( *name )
		std::strcat( name, " " );
	
	std::strcat( name, title );
}

static bool get_album_name( const FSRef& path, char* name, char* game )
{
	// to do: avoid creating emulator and all that?
	// to do: scan all tracks to be sure game name matches?
	
	BOOST::scoped_ptr<Music_Album> album( load_music_album( path ) );
	if ( !album )
		return false;
	
	const long sample_rate = 44100;
	Mono_Buffer buf;
	throw_if_error( buf.set_sample_rate( sample_rate, 10 ) );
	BOOST::scoped_ptr<Music_Emu> emu( album->make_emu( sample_rate, &buf ) );
	album->load_track( *emu, 0 );
	if ( album->track_count() == 1 )
	{
		// single-track album: name after track
		std::strcpy( name, album->info().song );
		std::strcpy( game, album->info().game );
	}
	else if ( album->track_count() > 1 )
	{
		// multi-track album: name after game
		std::strcpy( name, album->info().game );
		std::strcpy( game, name ); // set game so parent directory won't get renamed
	}
	
	return true;
}

static void retitle_item( const Cat_Info& info, Action_Hooks& hooks )
{
	// Build file list
	pod_vector<FSRef> files;
	int rename_dir = -1;
	if ( info.is_dir() )
	{
		rename_dir = 0;
		for ( Dir_Iterator iter( info.ref() ); iter.next(); )
		{
			if ( !iter.is_dir() )
				files.push_back( iter.ref() );
		}
	}
	else {
		files.push_back( info.ref() );
	}
	
	char dir_name [64];
	dir_name [0] = 0;
	OSType single_type = 0;
	
	// rename files
	for ( int i = 0; i < files.size(); i++ )
	{
		const FSRef& path = files [i];
		
		try
		{
			Cat_Info info;
			info.read( path, kFSCatInfoNodeFlags | kFSCatInfoFinderInfo );
			
			if ( info.is_alias() )
				continue;
			
			OSType type = is_music_or_archive( info.finfo().fileType );
			if ( !type )
				continue;
			
			if ( !hooks.advance( path ) )
				continue;
			
			// get song and game name
			char song [128];
			char game [128];
			*song = 0;
			*game = 0;
			switch ( type )
			{
				case spc_type:
				case spcp_type: get_spc_info( path, song, game ); break;
				
				default:
					if ( !get_album_name( path, song, game ) )
					{
						throw_error( fix_music_file_type( info, false ) );
						throw_error( "This file doesn't contain any game music" );
					}
					break;
			}
			
			if ( *game )
			{
				// don't rename dir if more than one game is named in files
				if ( !*dir_name )
					std::strcpy( dir_name, game );
				
				if ( rename_dir >= 0 && 0 == std::strcmp( dir_name, game ) )
					rename_dir++;
				else
					rename_dir = -1;
			}
			
			if ( *song )
			{
				// rename file
				song [max_filename] = 0;
				
				// preserve extension
				char old [256];
				get_filename( path, old, sizeof old );
				const char* ext = get_filename_extension( old );
				if ( ext ) {
					int len = std::strlen( ext );
					song [max_filename - len] = 0;
					std::strcat( song, ext );
				}
				
				HFSUniStr255 filename;
				str_to_filename( song, filename );
				sanitize_filename( filename );
				filename_to_str( filename, song );
				
				if ( 0 != std::strcmp( song, old ) )
					throw_file_error( FSRenameUnicode( &path, filename.length,
							filename.unicode, 0, NULL ), path );
			}
		}
		catch ( ... ) {
			hooks.log_exception( path );
		}
	}
	
	if ( rename_dir > 3 && *dir_name )
	{
		dir_name [max_filename] = 0;
		HFSUniStr255 filename;
		str_to_filename( dir_name, filename );
		sanitize_filename( filename );
		
		char old_name [256];
		get_filename( info.ref(), old_name, sizeof old_name );
		
		if ( 0 != std::strcmp( dir_name, old_name ) )
			throw_file_error( FSRenameUnicode( &info.ref(), filename.length,
					filename.unicode, 0, NULL ), info.ref() );
	}
}

void retitle_music( const FSRef& path, Action_Hooks& hooks )
{
	Cat_Info info;
	info.read( path, kFSCatInfoNodeFlags | kFSCatInfoFinderInfo );
	retitle_item( info, hooks );
}


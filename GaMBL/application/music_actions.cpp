
// Game_Music_Box 0.5.2. http://www.slack.net/~ant/game-music-box

#ifdef GMB_COMPILE_FILEIO

#include "music_actions.h"

//#include <LaunchServices.h>

#include "Multi_Buffer.h"
#include "Music_Album.h"
#include "Music_Emu.h"

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

// Number progress steps to use for music file/archive type so that progress
// is more consistent. Larger for archives or multi-track types.
static int estimate_count( OSType type )
{
	int result = 1;
	if ( type == rar_type )
		result = 15; // assume lots of files
	
	if ( type == nsf_type || type == nsfe_type || type == gbs_type )
		result = 15; // assume lots of tracks
	
	if ( type == zip_type )
		result = 5; // assume fewer files in a zip archive
	
	return result;
}

// Remove (OS 9)/hide (OS X) file extension. Ignores errors.
static void remove_filename_extension( const GaMBLFileHandle& path )
{
    debug_if_error( LSSetExtensionHiddenForRef( &path, true ) );
}

static void associate_item( const Cat_Info& info, Action_Hooks& hooks, bool hide_ext )
{
	// build list of files to rename
	pod_vector<GaMBLFileHandle> files;
	
	if ( info.is_dir() )
	{
		for ( Dir_Iterator iter( info.ref() ); iter.next(); )
		{
			associate_item( iter, hooks, hide_ext );
			if ( hooks.give_time() )
				return;
		}
	}
	else if ( !info.is_alias() )
	{
		try
		{
			OSType type = identify_music_file( info.ref(), info.finfo().fileType );
			if ( type && hooks.advance( info.ref(), estimate_count( type ) ) )
			{
				const char* error = NULL;
				if ( info.finfo().fileCreator != gmb_creator )
				{
					error = fix_music_file_type( info );
					if ( error )
						hooks.log_error( info.ref(), error );
				}
				
				if ( !error && hide_ext )
					files.push_back( info.ref() );
			}
		}
		catch ( ... )
		{
			hooks.log_exception( info.ref() );
		}
	}
	
	// rename files in list
	if ( !files.empty() )
	{
		for ( int i = 0; i < files.size(); i++ )
			remove_filename_extension( files [i] );
	}
}

void associate_music( const GaMBLFileHandle& path, Action_Hooks& hooks, bool hide_ext )
{
	Cat_Info info;
	info.read( path, kFSCatInfoNodeFlags | kFSCatInfoFinderInfo );
	associate_item( info, hooks, hide_ext );
}

// check_music_files

static void check_music_file( const GaMBLFileHandle& orig_path, const Cat_Info& info, Action_Hooks& hooks )
{
	OSType type = identify_music_file( info.ref(), info.finfo().fileType );
	if ( !type )
		return;
	
	if ( !hooks.advance( info.ref(), estimate_count( type ) ) )
		return;
	
	const char* error = fix_music_file_type( info, false );
	if ( error ) {
		hooks.log_error( info.ref(), error );
		return;
	}
	
	const int duration = 1000 / 10; // 1/10 second
	const long sample_rate = 44100;
	
	runtime_array<Music_Emu::sample_t> buf( sample_rate * duration / 1000 * 2 );
	
	unique_ptr<Music_Album> album( load_music_album( info.ref() ) );
	if ( !album ) {
		check( false );
		return;
	}
	
	Mono_Buffer bbuf;
	throw_if_error( bbuf.set_sample_rate( sample_rate, duration + 1 ) );
	unique_ptr<Music_Emu> emu( album->make_emu( sample_rate, &bbuf ) );
	
	int track_count = album->track_count();
	HFSUniStr255 name;
	FSGetCatalogInfoChk( orig_path, 0, NULL, &name );
	int single_track = (info.is_alias() ? extract_track_num( name ) : 0);
	if ( single_track )
	{
		if ( single_track > track_count ) {
			hooks.log_error( orig_path, "Track number isn't valid" );
			return;
		}
		track_count = single_track;
		single_track--;
	}
	else {
		track_count = album->track_count();
	}
	
	for ( int i = single_track; i < track_count; i++ )
	{
		int t = album->load_track( *emu, i );
		emu->start_track( t );
		
		emu->play( buf.size(), buf.begin() );
		if ( emu->error_count() > 0 )
			throw_if_error( "Emulation error" );
		
		if ( hooks.give_time() )
			return;
	}
}

static void check_item( const Cat_Info& info_in, Action_Hooks& hooks, int depth )
{
	if ( depth > 10 )
		throw_error( "Folders nested too deeply" );
	
	GaMBLFileHandle path = info_in.ref();
	
	try
	{
		Cat_Info new_info;
		bool use_new_info = !info_in.is_dir() && info_in.is_alias();
		const Cat_Info& info = use_new_info ? new_info : info_in;
		
		if ( use_new_info ) {
			new_info.read( DeprecatedFSResolveAliasFileChk( info_in.ref() ),
					kFSCatInfoNodeFlags | kFSCatInfoFinderInfo );
			path = new_info.ref();
		}
		
		if ( info.is_dir() )
		{
			for ( Dir_Iterator iter( info.ref() ); iter.next(); )
			{
				check_item( iter, hooks, depth + 1 );
				if ( hooks.give_time() )
					break;
			}
		}
		else
		{
			check_music_file( info_in.ref(), info, hooks );
		}
	}
	catch ( ... )
	{
		hooks.log_exception( path );
	}
}

void check_music_files( const GaMBLFileHandle& path, Action_Hooks& hooks )
{
	Cat_Info info;
	info.read( path, kFSCatInfoNodeFlags | kFSCatInfoFinderInfo );
	check_item( info, hooks, 0 );
}

#endif // #ifdef GMB_COMPILE_FILEIO



// Game_Music_Box 0.5.2. http://www.slack.net/~ant/game-music-box

#include "favorites.h"
#include "FileUtilities.h"

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

const char* system_names [5] = {
	"Nintendo (NES)",
	"Game Boy",
	"Master System",
	"Super Nintendo",
	"Sega Genesis"
};

static GaMBLFileHandle create_dir( const GaMBLFileHandle& parent, const char* name )
{
    assert(0);
#if 0 //RAD
	HFSUniStr255 filename;
	str_to_filename( name, filename );
	sanitize_filename( filename );
	GaMBLFileHandle dir;
	if ( !FSMakeFSRefExists( parent, filename, &dir ) )
		FSCreateDirectoryUnicodeChk( parent, filename, 0, NULL, &dir );
	else
		dir = DeprecatedFSResolveAliasFileChk( dir );
	return dir;
#else
    return DummyHandle;
#endif
}

const GaMBLFileHandle& favorites_dir()
{
	static GaMBLFileHandle dir;
#if 0 //RAD
	static bool found;
	if ( !found )
	{
		if ( debug_if_error( DeprecatedFSFindFolder( kOnSystemDisk,
				kPreferencesFolderType, true, &dir ) ) )
		{
			dir = get_parent( get_bundle_fsref() );
		}
		dir = create_dir( dir, "Game Music Favorites" );
		found = true;
	}
#endif
	return dir;
}

void play_classic_favorites( Player_Window& player )
{
//TODO: GUI
#if 0
	player.begin_drop();
	GaMBLFileHandle dir = favorites_dir();
	player.handle_drop( create_dir( dir, system_names [0] ) );
	player.handle_drop( create_dir( dir, system_names [1] ) );
	player.handle_drop( create_dir( dir, system_names [2] ) );
	player.end_drop();
#endif
}

void play_favorites( Player_Window& player, const char* name )
{
    //TODO: GUI
#if 0
	player.begin_drop();
	GaMBLFileHandle dir = favorites_dir();
	player.handle_drop( create_dir( dir, name ) );
	player.end_drop();
#endif
}

void open_favorites()
{
	//launch_file( favorites_dir() );
}

void add_favorite( const track_ref_t& track, shared_ptr< Music_Album > album )
{
	// to do: currently just a quick hack
	
	GaMBLFileHandle dir = favorites_dir();
	
	dir = create_dir( dir, album->info().system );
	
	char name [PATH_MAX];
	strcpy_trunc( name, album->info().game, sizeof name );
	dir = create_dir( dir, name );
	
	strcpy_trunc( name, album->info().song, sizeof name );
	if ( !*name )
		strcpy_trunc( name, album->info().game, sizeof name );
	
	char alias_name [PATH_MAX];
	alias_name [0] = 0;
	
	if ( album->track_count() > 1 )
	{
		int track_len = (album->track_count() < 100 ? 2 : 3);
		
		name [PATH_MAX - track_len - 2] = 0;
		
		alias_name [0] = '#';
		num_to_str( track.track + 1, alias_name + 1, -track_len );
		std::strcat( alias_name, " " );
	}
	
	std::strcat( alias_name, name );
	
    std::wstring filename;
	str_to_filename( alias_name, filename );
	sanitize_filename( filename );
	
	// don't create if alias already exists
	if ( !FileExists( filename ) )
		CreateAlias( track, filename );
}



// Top-level app class and dispatch

// Game_Music_Box 0.5.2. http://www.slack.net/~ant/game-music-box

#include "common.h"

#include "Utility_Window.h"
#include "Player_Window.h"
#include "music_actions.h"
#include "Carbon_App.h"
#include "file_util.h"
#include "favorites.h"
#include "prefs.h"

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

#ifdef GMB_COMPILE_APP

class App : public Carbon_App {
	unique_ptr<Player_Window> player;
	unique_ptr<Utility_Window> utility;
public:
	
	App() {
		add_help_item( PROGRAM_NAME " Help", 'Help', '?' );
		add_help_item( "Game Music Overview", 'GMOv' );
	}
	
	void show_about()
	{
		if ( Alert( 128, NULL ) == 3 )
			launch_url( "http://www.slack.net/~ant/game-music-box/" );
	}
	
	void run_event_loop()
	{
		prefs.init();
		
		// show about box on first run
		if ( !prefs.exist )
			show_about();
		
		player.reset( new Player_Window );
		
		Carbon_App::run_event_loop();
		
		// destructors write new preferences
		utility.reset();
		player.reset();
		
		prefs.cleanup();
	}
	
	void app_activated()
	{
		if ( player && !player->visible() )
			player->show();
		
		app_to_front();
		Carbon_App::app_activated();
	}
	
	void app_deactivated()
	{
		if ( player )
			player->uncache();
	}
	
	void open_files( const FSRef*, int );
	
	void open_file( const FSRef& spec ) {
		player->handle_drop( spec );
	}
	
	void edit_preferences() {
		prefs.show();
	}
	
	bool handle_command( const HICommandExtended& );
	
#ifndef NDEBUG
	void opened()
	{
		// open "auto test" in current directory, if it exists
		try {
			HFSUniStr255 name;
			str_to_filename( "auto test", name );
			FSRef ref;
			if ( FSMakeFSRefExists( get_parent( get_bundle_fsref() ), name, &ref ) ) {
				ref = FSResolveAliasFileChk( ref );
				open_files( &ref, 1 );
			}
		}
		catch ( ... ) {
			check( false );
		}
	}
#endif
};

bool App::handle_command( const HICommandExtended& cmd )
{
	switch ( cmd.commandID )
	{
		case 'Util':
			if ( !utility )
				utility.reset( new Utility_Window( player.get() ) );
			utility->show();
			SelectWindow( utility->window() );
			return true;
		
		case 'Help':
			open_help_document( PROGRAM_NAME " Manual" );
			return true;
		
		case 'GMOv':
			open_help_document( "Game Music Overview" );
			return true;
		
		case 'OFav':
			open_favorites();
			return true;
		
		case 'DFav':
		case 'AFav':
			if ( player && player->album() )
				add_favorite( player->track(), *player->album() );
			return true;
		
		case 'Fav0':
		case 'Fav1':
		case 'Fav2':
		case 'Fav3':
		case 'Fav4':
			play_favorites( *player, system_names [cmd.commandID - 'Fav0'] );
			return true;
		
		case 'FavC':
			play_classic_favorites( *player );
			return true;
		
	}
	
	return Carbon_App::handle_command( cmd );
}

static const char* check_file( const FSRef& path, bool full_check )
{
	Cat_Info info;
	info.read( path, kFSCatInfoNodeFlags | kFSCatInfoFinderInfo );
	
	// ignore directory
	if ( info.is_dir() )
	{
		if ( full_check )
			return "No game music was found in the folder.";
		return NULL;
	}
	
	// shared data
	if ( info.finfo().fileType == shared_type )
		return "This file contains shared data for Super NES music "
				"and can't be opened directly.";
	
	OSType type = identify_music_file( path, info.finfo().fileType );
	
	// RAR archive
	#if !UNRAR_AVAILABLE
		if ( type == rar_type )
			return "RAR archives are not supported. Expand them first.";
	#endif
	
	// type identified and creator already set; fine
	if ( type && info.finfo().fileCreator == gmb_creator && !full_check )
		return NULL;
	
	// check/fix file type and report result
	return fix_music_file_type( info, full_check && prefs.change_icon );
}

void App::open_files( const FSRef* specs, int count )
{
	if ( count == 1 )
		throw_if_error( check_file( *specs, false ) );
	
	player->begin_drop();
	Carbon_App::open_files( specs, count );
	
	if ( !player->end_drop( true ) && count == 1 )
	{
		throw_if_error( check_file( *specs, true ) );
		throw_error( "No supported game music was found." );
	}
}

int main()
{
	run_carbon_app<App>();
	return 0;
}

#endif

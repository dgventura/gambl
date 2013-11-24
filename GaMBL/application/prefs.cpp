
// Game_Music_Box 0.5.2. http://www.slack.net/~ant/game-music-box



#include "prefs.h"

#include <algorithm>
//#include <ControlDefinitions.h>

//#include "Carbon_Window.h"
//#include "mac_controls.h"
#include "app_prefs.h"

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

App_Prefs prefs;

void App_Prefs::init()
{
	// defaults
	prefs.exist = false;
	
	// player
	prefs.shuffle = false;
	prefs.echo_depth = 0.2;
	prefs.treble = 0.5;
	prefs.bass = 0.5;
	prefs.custom_sound = false;
	prefs.disable_surround = false;
	prefs.volume = 0.5;
	prefs.duration = 1.0;
	prefs.show_info = true;
	prefs.change_icon = false;
	prefs.songs_only = false;
	
	// utility
	prefs.util.active = false;
	prefs.util.remove_extensions = false;
	
	// scope
	prefs.scope.zoomed = true;
	prefs.scope.visible = false;
	
	// read prefs
	//TODO prefs.exist = reflect_prefs( false );
}

#ifdef GMB_COMPILE_GUI
static bool reflect_prefs( bool set )
{
	int version = set ? 1 : 0;
	reflect_pref_int( set, &version, "version" );
	if ( version == 1 )
	{
		reflect_pref_int( set, &prefs.shuffle, "shuffle" );
		reflect_pref_int( set, &prefs.songs_only, "songs_only" );
		reflect_pref_int( set, &prefs.show_info, "show_info" );
		
		reflect_pref_int( set, &prefs.custom_sound, "custom_sound" );
		reflect_pref_float(set,&prefs.treble, "treble" );
		reflect_pref_float(set,&prefs.bass, "bass" );
		reflect_pref_float(set,&prefs.echo_depth, "echo_depth" );
		
		reflect_pref_int( set, &prefs.change_icon, "change_icon" );
		reflect_pref_int( set, &prefs.disable_surround, "disable_surround" );
		
		// utility
		reflect_pref_int( set, &prefs.util.remove_extensions, "util_remove_extensions" );
		
		// scope
		reflect_pref_int( set, &prefs.scope.zoomed, "scope_zoomed" );
		reflect_pref_int( set, &prefs.scope.visible, "scope_visible" );
		
		// window positions
		reflect_pref_int( set, &prefs.prefs_pos.h, "prefs_pos_h" );
		reflect_pref_int( set, &prefs.prefs_pos.v, "prefs_pos_v" );
		reflect_pref_int( set, &prefs.chans_pos.h, "chans_pos_h" );
		reflect_pref_int( set, &prefs.chans_pos.v, "chans_pos_v" );
		reflect_pref_int( set, &prefs.player_pos.h, "player_pos_h" );
		reflect_pref_int( set, &prefs.player_pos.v, "player_pos_v" );
		reflect_pref_int( set, &prefs.util.window_pos.h, "util_window_pos_h" );
		reflect_pref_int( set, &prefs.util.window_pos.v, "util_window_pos_v" );
		reflect_pref_int( set, &prefs.errors_pos.h, "errors_pos_h" );
		reflect_pref_int( set, &prefs.errors_pos.v, "errors_pos_v" );
		reflect_pref_int( set, &prefs.scope.window_pos.h, "scope_window_pos_h" );
		reflect_pref_int( set, &prefs.scope.window_pos.v, "scope_window_pos_v" );
	}
	return (version != 0);
}

struct Prefs_Window : Carbon_Window
{
	Value_Control change_icon_box;
	Value_Control disable_surround_box;
	
	Prefs_Window();
	
	~Prefs_Window() {
		prefs.prefs_pos = Carbon_Window::top_left();
	}
	
	bool handle_command( const HICommandExtended& cmd )
	{
		switch ( cmd.commandID )
		{
			case 'Clos':
				show( false );
				return true;
			
			case 'Updt':
				prefs.change_icon = change_icon_box.value();
				prefs.disable_surround = disable_surround_box.value();
				return true;
		}
		
		return Carbon_Window::handle_command( cmd );
	}
};

static unique_ptr<Prefs_Window> prefs_window;

const int window_width = 250;

Prefs_Window::Prefs_Window()
{
	Carbon_Window::make( kDocumentWindowClass, kWindowCloseBoxAttribute |
			kWindowStandardHandlerAttribute );
	Carbon_Window::set_title( "Preferences" );
	
	Row_Placer rows( window_width );
	
	change_icon_box.make_checkbox( window(), rows.place( 20 ), "Update File Icon When Played", 'Updt' );
	change_icon_box.set_value( prefs.change_icon );
	set_control_help( change_icon_box, "Automatically changes icons of music files when they are played" );
	
	disable_surround_box.make_checkbox( window(), rows.place( 20 ), "Disable SNES Surround", 'Updt' );
	disable_surround_box.set_value( prefs.disable_surround );
	set_control_help( disable_surround_box, "Disables surround effect used in some SNES soundtracks" );
	
	SizeWindow( window(), window_width, rows.bounds().bottom + 4, true );
	
	RepositionWindow( window(), NULL, kWindowCenterOnMainScreen );
	Carbon_Window::restore_top_left( prefs.prefs_pos );
}

void App_Prefs::show()
{
	if ( !prefs_window )
		prefs_window.reset( new Prefs_Window );
	ShowWindow( prefs_window->window() );
	SelectWindow( prefs_window->window() );
}

void App_Prefs::cleanup()
{
	prefs_window.reset();
	reflect_prefs( true );
	update_app_prefs();
}

#endif // #ifdef GMB_COMPILE_GUI



// Game_Music_Box 0.5.2. http://www.slack.net/~ant/game-music-box

#include "Channel_Window.h"

#include "Player_Window.h"
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

const int window_width = 215;

Channel_Window::Channel_Window( Player_Window* p ) : player( *p )
{
	chan_count = 0;
	chan_names = NULL;
	allow_custom_sound_ = false;
}

Channel_Window::~Channel_Window()
{
	if ( window() )
		prefs.chans_pos = Carbon_Window::top_left();
}

void Channel_Window::make_window()
{
	make( kFloatingWindowClass, kWindowCloseBoxAttribute | kWindowStandardHandlerAttribute );
	set_title( "Sound Channels" );
	
	Row_Placer rows( window_width );
	
	const int slider_width = window_width - 10;
	eq_checkbox.make_checkbox( window(), rows.place( 20 ), "Customize Sound", 'Updt', true );
	eq_checkbox.set_value( prefs.custom_sound );
	set_control_help( eq_checkbox, "Applies only to Nintendo, Game Boy, and Master System" );
	
	treble_slider.make( window(), rows.place( 20, slider_width ),
			"Treble", 60, 0, handle_slider, this );
	treble_slider.set_fraction( prefs.treble );
	
	bass_slider.make( window(), rows.place( 20, slider_width ),
			"Bass", 60, 0, handle_slider, this );
	bass_slider.set_fraction( prefs.bass );
	
	echo_depth.make( window(), rows.place( 20, slider_width ),
			"Stereo", 60, 0, handle_slider, this );
	echo_depth.set_fraction( prefs.echo_depth );
	set_control_help( echo_depth, "Spreads center channels and adds echo" );
	
	collapsed_height = rows.bounds().bottom + 4;
	
	for ( int rn = 0; rn < max_chan / 2; rn++ )
	{
		Row_Filler row( rows.place( 20 ) );
		rect r = row.place_left( 85 );
		
		for ( int phase = 0; phase < 2; phase++ )
		{
			int i = phase * (max_chan / 2) + rn;
			chan_checkboxes [i].make_checkbox( window(), r, "", 'Updt', true );
			chan_checkboxes [i].set_value( true );
			r = row.remain();
		}
	}
	
	expanded_height = rows.bounds().bottom + 4;
	
	int count = chan_count;
	chan_count = !count; // force resize
	set_names( chan_names, count );
	
	RepositionWindow( window(), NULL, kWindowCenterOnMainScreen );
	Carbon_Window::restore_top_left( prefs.chans_pos );
	
	update_eq_enables();
}

void Channel_Window::update_channels()
{
	int mask = 0;
	for ( int i = 0; i < max_chan; i++ )
		mask |= (chan_checkboxes [i].value() ^ 1) << i;
	player.set_mute( 0x8000 | mask );
}

void Channel_Window::update_eq_enables()
{
	if ( !window() )
		return;
	
	bool b = allow_custom_sound_;
	eq_checkbox.enable( b );
	echo_depth.enable( b && eq_checkbox.value() );
	b &= prefs.custom_sound;
	treble_slider.enable( b );
	bass_slider.enable( b );
}

void Channel_Window::prefs_changed()
{
	if ( !window() )
		return;
	
	prefs.custom_sound = eq_checkbox.value();
	prefs.treble = treble_slider.fraction();
	prefs.bass = bass_slider.fraction();
	prefs.echo_depth = echo_depth.fraction();
	if ( prefs.echo_depth < 0.01 )
		prefs.echo_depth = 0.0; // ensure 0.0 is easily reachable
	update_eq_enables();
	
	player.setup_changed();
}

void Channel_Window::handle_slider( void* self, Slider_Control* slider, int part )
{
	if ( part ) {
		double value = slider->fraction();
		value += part * 0.1;
		if ( value < 0 )
			value = 0;
		else if ( value > 1.0 )
			value = 1.0;
		slider->set_fraction( value );
	}
	
	static_cast<Channel_Window*> (self)->prefs_changed();
}

bool Channel_Window::handle_command( const HICommandExtended& cmd )
{
	switch ( cmd.commandID )
	{
		case 'Clos':
			show( false );
			return true;
		
		case 'Updt':
			update_channels();
			prefs_changed();
			return true;
	}
	
	return Carbon_Window::handle_command( cmd );
}

void Channel_Window::enable_eq( bool b )
{
	allow_custom_sound_ = b;
	update_eq_enables();
}

void Channel_Window::show( bool b )
{
	if ( !window() ) {
		if ( !b )
			return;
		make_window();
	}
	
	Carbon_Window::show( b );
	if ( b ) {
		SelectWindow( window() );
		update_channels();
	}
	else {
		player.set_mute( 0 );
	}
}

void Channel_Window::set_names( const char** names, int count )
{
	require( count <= max_chan );
	
	if ( !chan_count != !count )
		SizeWindow( window(), window_width, (count ? expanded_height : collapsed_height), true );
	
	chan_names = names;
	chan_count = count;
	
	for ( int i = 0; i < max_chan; i++ )
	{
		if ( i < count ) {
			chan_checkboxes [i].show( true );
			chan_checkboxes [i].set_title( names [i] );
		}
		else {
			chan_checkboxes [i].show( false );
		}
	}
}



// Game_Music_Box 0.5.2. http://www.slack.net/~ant/game-music-box

#include "Progress_Window.h"

//#include <ControlDefinitions.h>
#include "Carbon_App.h"

/* This module copyright (C) 2005 Shay Green. This module is free software;
you can redistribute it and/or modify it under the terms of the GNU Lesser
General Public License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version. This
module is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
more details. You should have received a copy of the GNU Lesser General
Public License along with this module; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA */

// Progress_Hook

Progress_Hook::Progress_Hook() {
	count = 0;
	scale = 0.0;
}

void Progress_Hook::set_total( int n ) {
	scale = (n ? 1.0 / n : 0.0);
}

void Progress_Hook::advance( int n )
{
	count += n;
	update( 0.0 );
}

void Progress_Hook::update( double progress ) {
	set_progress( count * scale + progress * scale );
}

void Progress_Hook::set_text( const char* ) {
}

bool Progress_Hook::has_time_passed()
{
	return false;
}

bool Progress_Hook::give_time() {
	return CheckEventQueueForUserCancel();
}

// Progress_Window

Progress_Window::Progress_Window( const char* title, bool modal )
{
	text [0] = 0;
	next_time = 0;
	button = NULL;
	progress = 0.0;
	time_passed = false;
	progress_known_ = false;
	was_stopped_ = false;
	show_time = TickCount() + 20;
	
	make( modal ? kMovableModalWindowClass : kDocumentWindowClass,
			kWindowStandardHandlerAttribute );
	set_title( title );
}

Progress_Window::~Progress_Window() {
}

void Progress_Window::set_text( const char* str ) {
	std::strncpy( text, str, sizeof text );
	text [(sizeof text - 1)] = 0;
}

bool Progress_Window::handle_command( const HICommandExtended& cmd )
{
	switch ( cmd.commandID )
	{
		case 'Stop':
			was_stopped_ = true;
			return true;
		
		case 'Clos':
			// ignore close command
			return true;
	}
	
	return Carbon_Window::handle_command( cmd );
}

bool Progress_Window::handle_event( EventRef event )
{
	if ( IsUserCancelEventRef( event ) ) {
		if ( !was_stopped_ ) {
			was_stopped_ = true;
			if ( button )
				press_button( button ); 
		}
		return true;
	}
	
	return Carbon_Window::handle_event( event );
}

bool Progress_Window::was_stopped() const {
	return was_stopped_ || Carbon_App::is_quitting();
}

void Progress_Window::show()
{
	if ( !show_time )
		return;
	show_time = 0;
	
	Row_Placer rows( 275 );
	
	rect r = rows.place( 20 );
	r.right = r.left + 350;
	status_text.make( window(), r );
	
	{
		Row_Filler row( rows.place( 16 ) );
		
		ControlRef ref;
		debug_if_error( CreateProgressBarControl( window(), row.place_left( 200 ),
				0, 0, 0x1000, progress == 0.0, &ref ) );
		bar.reset( ref );
		
		row.place_left( 4 ); // progress bar needs extra
		
		button = make_push_button( window(), row.remain( 20 ), "Stop", 'Stop' );
	}
	
	SizeWindow( window(), rows.bounds().right + 4, rows.bounds().bottom + 6, true );
	RepositionWindow( window(), NULL, kWindowAlertPositionOnMainScreen );
	Carbon_Window::show();
}

bool Progress_Window::has_time_passed()
{
	bool result = time_passed;
	time_passed = false;
	return result;
}

static void give_time()
{
	for ( int n = 12; n--; )
	{
		EventRef event;
		OSStatus result = ReceiveNextEvent( 0, NULL, kEventDurationSecond / 100, true, &event );
		if ( result == eventLoopTimedOutErr )
			break;
		if ( !debug_if_error( result ) ) {
			SendEventToEventTarget( event, GetEventDispatcherTarget() );
			ReleaseEvent( event );
		}
	}
}

bool Progress_Window::give_time()
{
	unsigned long time = TickCount();
	if ( time >= next_time )
	{
		next_time = time + 10;
		time_passed = true;
		
		if ( show_time && time > show_time )
			show();
		
		if ( !show_time )
		{
			if ( *text ) {
				status_text.set_text( text );
				*text = 0;
			}
			double p = progress;
			if ( p > 1.0 )
				p = 1.0;
			
			if ( !progress_known_ && p > 0.0 )
			{
				progress_known_ = true;
				Boolean b = false;
				debug_if_error( SetControlData( bar, kControlIndicatorPart,
						kControlProgressBarIndeterminateTag, sizeof b, &b ) );
			}
			
			if ( progress_known_ )
				bar.set_value( p * 0x1000 );
		}
		idle();
		::give_time();
	}
	return was_stopped();
}

void Progress_Window::idle() {
	IdleControls( window() );
}



// Game_Music_Box 0.5.2. http://www.slack.net/~ant/game-music-box

#include "Carbon_Window.h"

#include "mac_util.h"
//#include <Strings.h>

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

const Rect Carbon_Window::default_bounds = { 0, 0, 100, 100 };

Carbon_Window::Carbon_Window() {
	window_ = NULL;
}

void Carbon_Window::make( WindowClass c, WindowAttributes a, const Rect& r )
{
	throw_if_error( CreateNewWindow( c, a, &r, &window_ ) );
	
	//static const RGBColor bg_color = { 0xEEEE, 0xEEEE, 0xEEEE };
	//SetWindowContentColor( window(), &bg_color );
	
	static const EventTypeSpec handlers [] = {
		//{ kEventClassWindow, kEventWindowProxyBeginDrag },
		//{ kEventClassWindow, kEventWindowProxyEndDrag },
		//{ kEventClassWindow, kEventWindowClickProxyIconRgn },
		{ kEventClassWindow, kEventWindowClose },
		{ kEventClassWindow, kEventWindowZoom },
		{ kEventClassWindow, kEventWindowBoundsChanged },
		{ kEventClassWindow, kEventWindowActivated },
		{ kEventClassWindow, kEventWindowDeactivated },
		{ kEventClassWindow, kEventWindowGetClickActivation },
		{ kEventClassCommand, kEventCommandProcess },
		{ kEventClassKeyboard, kEventRawKeyDown },
		{ kEventClassKeyboard, kEventRawKeyRepeat },
		{ kEventClassWindow, kEventWindowDrawContent }
	};
	debug_if_error( InstallWindowEventHandler( window_, event_handler,
			GetEventTypeCount( handlers ), handlers, event_handler_data(), NULL ) );
}

Carbon_Window::~Carbon_Window()
{
	if ( window_ )
		DisposeWindow( window_ );
}

void Carbon_Window::set_title( const char* s )
{
	Str255 old;
	GetWTitle( window(), old );
	Str255 p;
	c2pstrcpy( p, s );
	
	if ( 0 != std::memcmp( old, p, p [0] + 1 ) )
		SetWTitle( window(), p );
}

void Carbon_Window::show( bool b ) {
	if ( b )
		ShowWindow( window() );
	else
		HideWindow( window() );
}

bool Carbon_Window::visible() const {
	return MacIsWindowVisible( window() );
}

bool Carbon_Window::handle_event( EventRef event )
{
	switch ( GetEventClass( event ) )
	{
		case kEventClassWindow:
		{
			long commandID = 0;
			switch ( GetEventKind( event ) )
			{
				case kEventWindowBoundsChanged: {
					rect r;
					debug_if_error( GetWindowBounds( window(), kWindowContentRgn, &r ) );
					bounds_changed( r.width(), r.height() );
					return true;
				}
				
				case kEventWindowDrawContent:
					draw_content();
					break;
				
				case kEventWindowClose:
					commandID = 'Clos';
					break;
				
				case kEventWindowZoom:
					commandID = 'Zoom';
					break;
				
				case kEventWindowActivated:
					this->event_handled = true;
					window_activated();
					return this->event_handled;
				
				case kEventWindowDeactivated:
					this->event_handled = true;
					window_deactivated();
					return this->event_handled;
				
				case kEventWindowGetClickActivation:
					return handle_clickthrough( event );
			}
			
			HICommandExtended cmd;
			cmd.attributes = 0;
			cmd.commandID = commandID;
			if ( commandID && handle_command( cmd ) )
				return true;
		}
	}
	
	return false;
}

Point Carbon_Window::top_left() const
{
	require( window() );
	Rect r;
	GetWindowBounds( window(), kWindowGlobalPortRgn, &r );
	return point( r.left, r.top );
}

void Carbon_Window::restore_top_left( const Point& pt )
{
	require( window() );
	
	if ( !pt.h && !pt.v )
		return;
	
	// get title bar rgn at proposed position
	RgnHandle rgn = NewRgn();
	GetWindowRegion( window(), kWindowTitleBarRgn, rgn );
	Point tl = top_left();
	OffsetRgn( rgn, pt.h - tl.h, pt.v - tl.v );
	
	// move window if title bar would be at all visible
	SectRgn( GetGrayRgn(), rgn, rgn );
	if ( !EmptyRgn( rgn ) )
		MacMoveWindow( window(), pt.h, pt.v, false );
	
	DisposeRgn( rgn );
}

void Carbon_Window::window_activated()
{
	event_handled = false;
}

void Carbon_Window::window_deactivated()
{
	event_handled = false;
}

bool Carbon_Window::handle_clickthrough( EventRef )
{
	return false;
}

void Carbon_Window::bounds_changed( int, int )
{
}

void Carbon_Window::draw_content()
{
}


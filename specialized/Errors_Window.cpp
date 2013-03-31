
// Game_Music_Box 0.5.2. http://www.slack.net/~ant/game-music-box

#include "Errors_Window.h"

//#include <ControlDefinitions.h>
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

#include "source_begin.h"

const int window_width = 650;

Errors_Window::Errors_Window( const char* title_, const char* text_ )
{
	text = text_;
	title = title_;
	list_box = NULL;
	list_ref = NULL;
	error_count = 0;
}

Errors_Window::~Errors_Window()
{
	if ( list_box )
		DisposeControl( list_box );
}

bool Errors_Window::make_window()
{
	if ( error_count == 0 )
		return false;
	
	if ( window() )
		return true;
	
	Carbon_Window::make( kDocumentWindowClass, kWindowStandardHandlerAttribute |
			kWindowResizableAttribute | kWindowCloseBoxAttribute |
			(is_os_x() ? kWindowLiveResizeAttribute : 0) );
	Carbon_Window::set_title( title );
	
	Row_Placer rows( window_width, point( 0, 0 ) );
	rows.set_spacing( 0 );
	
	// Required on OS 9 or list box scrollbars won't work
	ControlRef root;
	CreateRootControl( window(), &root );
	
	rows.place( 4 );
	rect r = rows.place( 16 * 1 + 10 );
	r.left += 4;
	r.right -= 4;
	make_static_text( window(), r, text );
	
	ControlFontStyleRec style;
	style.flags = kControlUseFontMask | kControlUseFaceMask | kControlUseSizeMask;
	style.font = kFontIDCourier;
	style.style = condense;
	style.size = 14;
	
	static const ListDefSpec spec = { kListDefStandardTextType };
	throw_if_error( CreateListBoxControl( window(), rows.place( 16 * 14, window_width ),
			false, 0, 1, false, true, 16, window_width, false, &spec, &list_box ) );
	rows.place( 15 );
	
	debug_if_error( SetControlFontStyle( list_box, &style ) );
	
	throw_if_error( GetControlData( list_box, kControlListBoxPart,
			kControlListBoxListHandleTag, sizeof list_ref, &list_ref, NULL ) );
	SetListSelectionFlags( list_ref, lNoNilHilite | lOnlyOne );
	
	old_width = window_width;
	old_height = rows.bounds().bottom;
	
	SizeWindow( window(), old_width, old_height, true );
	
	RepositionWindow( window(), NULL, kWindowCenterOnMainScreen );
	
	return true;
}

void Errors_Window::add_error( const char* str )
{
	error_count++;
	make_window();
	LAddRow( 1, error_count - 1, list_ref );
	LSetCell( str, std::strlen( str ), point( 0, error_count - 1 ), list_ref );
	if ( visible() )
		Draw1Control( list_box );
}

void Errors_Window::bounds_changed( int width, int height )
{
	rect r;
	GetControlBounds( list_box, &r );
	
	int h_offset = width - old_width;
	r.right += h_offset;
	r.bottom += height - old_height;
	
	// OS X doesn't move scrollbar horizontally when list is resized
	ControlRef scrollbar = NULL;
	if ( is_os_x() ) {
		scrollbar = GetListVerticalScrollBar( list_ref );
		check( scrollbar );
	}
	
	rect sb_rect;
	if ( scrollbar )
		GetControlBounds( scrollbar, &sb_rect );
	
	LCellSize( point( r.width(), 16 ), list_ref );
	SizeControl( list_box, r.width(), r.height() );
	
	if ( scrollbar )
		MoveControl( scrollbar, sb_rect.left + h_offset, sb_rect.top );
	
	// These don't help
	//SetListViewBounds( list_ref, r );
	//SetControlBounds( list_box, r );
	
	old_width = width;
	old_height = height;
}

bool Errors_Window::handle_command( const HICommandExtended& cmd )
{
	switch ( cmd.commandID )
	{
		case 'Clos':
			show( false );
			// to do: optionally delete self
			return true;
	}
	
	return Carbon_Window::handle_command( cmd );
}

void Errors_Window::clear()
{
	if ( list_ref )
	{
		rect r;
		GetListDataBounds( list_ref, &r );
		if ( r.height() )
		{
			LDelRow( r.height(), 0, list_ref );
			Draw1Control( list_box );
		}
	}
	error_count = 0;
}

void Errors_Window::add_file_error_( const FSRef& path, const char* error )
{
	// to do: handle path to a disk (i.e. nothing above it)
	char str [exception_str_max];
	str [0] = 0;
	try {
		get_filename( get_parent( path ), str, 256 );
		std::strcat( str, "/" );
		get_filename( path, str_end( str ), 256 );
	}
	catch ( ... ) {
		check( false );
	}
	
	const int path_max = 32;
	
	int len = std::strlen( str );
	if ( len > path_max )
	{
		std::memmove( str, str + len - path_max, path_max + 1 );
		len = path_max;
		str [0] = 'É';
	}
	
	const int spaces = 4;
	std::memset( str + len, ' ', path_max + spaces - len );
	str [path_max + spaces] = 0;
	
	if ( error ) {
		std::strcat( str, error );
	}
	else
	{
		static const char generic_error [] = "An error occurred";
		
		try {
			throw;
		}
		catch ( File_Error& e )
		{
			if ( noErr == FSCompareFSRefs( &e.fsref, &path ) )
			{
				const char* estr = error_to_str( e.code );
				if ( !estr )
					estr = e.what();
				if ( !estr )
					estr = generic_error;
				std::strcat( str_end( str ), estr );
			}
			else if ( !exception_to_str( str_end( str ) ) ) {
				std::strcat( str, generic_error );
			}
		}
		catch ( ... )
		{
			if ( !exception_to_str( str_end( str ) ) )
				std::strcat( str, generic_error );
		}
	}
	
	add_error( str );
}


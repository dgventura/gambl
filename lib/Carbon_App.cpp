
// Game_Music_Box 0.5.2. http://www.slack.net/~ant/game-music-box

#include "Carbon_App.h"

//#include <Gestalt.h>
#include <memory>

#include "mac_util.h"

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

#ifdef GMB_COMPILE_APP

bool Carbon_App::is_quitting_;
Carbon_App* Carbon_App::app_;

Carbon_App::Carbon_App() {
	app_ = this;
}

Carbon_App::~Carbon_App() {
	app_ = NULL;
}

pascal OSErr Carbon_App::ae_open_files( const AppleEvent* event, AppleEvent* reply, long )
{
	OSErr err;
	
	AEDescList list;
	err = AEGetParamDesc( event, keyDirectObject, typeAEList, &list );
	if ( err )
		return err;
	
	FSRef* specs = NULL;
	
	DescType type;
	Size size;
	err = AEGetAttributePtr( event, keyMissedKeywordAttr, typeWildCard, &type,
			NULL, 0, &size );
	if ( err != errAEDescNotFound ) {
		if ( !err )
			err = errAEParamMissed;
		goto exit;
	}
	
	long count;
	count = 0;
	EXIT_IF_ERROR( AECountItems( &list, &count ) );
	
	specs = new (std::nothrow) FSRef [count];
	if ( !specs )
		EXIT_IF_ERROR( memFullErr );
	
	// build array of specs
	for ( int i = 0; i < count; i++ ) {
		AEKeyword keyword;
		Size size;
		EXIT_IF_ERROR( AEGetNthPtr( &list, i + 1, typeFSRef, &keyword, &type,
				&specs [i], sizeof specs [i], &size ) );
	}
	
	// open specs
	try {
		Carbon_App::app()->open_files( specs, count );
	}
	catch ( ... ) {
		EXIT_IF_ERROR( report_exception() );
	}
	
	err = noErr;
	
exit:
	delete [] specs;
	AEDisposeDesc( &list );
	
	return err;
}

pascal OSErr Carbon_App::ae_open( const AppleEvent*, AppleEvent*, long app ) {
	try {
		Carbon_App::app()->opened();
	}
	catch ( ... ) {
		return report_exception();
	}
		
	return 0;
}

static pascal OSErr ae_print_files( const AppleEvent*, AppleEvent*, long app ) {
	// to do
	return errAEEventNotHandled;
}

static pascal OSErr ae_quit( const AppleEvent*, AppleEvent*, long app ) {
	try {
		if ( Carbon_App::is_quitting() )
			QuitApplicationEventLoop();
		else
			send_quit_event();
	}
	catch ( ... ) {
		return report_exception();
	}
		
	return 0;
}

void Carbon_App::init_events()
{
	install_ae_handler( kAEOpenApplication, ae_open );
	install_ae_handler( kAEOpenDocuments,   ae_open_files );
	install_ae_handler( kAEPrintDocuments,  ae_print_files );
	install_ae_handler( kAEQuitApplication, ae_quit );
	
	static const EventTypeSpec types [] = {
		{ kEventClassCommand, kEventCommandProcess },
		{ kEventClassApplication, kEventAppActivated },
		{ kEventClassApplication, kEventAppDeactivated }
	};
	debug_if_error( InstallApplicationEventHandler( event_handler,
			GetEventTypeCount( types ), types, event_handler_data(), NULL ) );
}

void Carbon_App::init_menus()
{
	SetMenuBar( throw_if_null( GetNewMBar( 128 ) ) );
	
	if ( is_aqua_ui() )
	{
		// Aqua UI; use its Quit and Preferences menus
		
		// Remove quit from file menu
		{
			MenuRef menu = throw_if_null( GetMenuHandle( 129 ) );
			int count = CountMenuItems( menu );
			if ( count > 2 ) {
				// remove separator and quit item
				DeleteMenuItem( menu, count - 1 );
				DeleteMenuItem( menu, count - 1 );
			}
			else {
				// remove entire file menu
				MacDeleteMenu( 129 );
			}
		}
		
		// Remove preferences from edit menu
		{
			MenuRef menu = throw_if_null( GetMenuHandle( 130 ) );
			int count = CountMenuItems( menu );
			MenuCommand cmd;
			debug_if_error( GetMenuItemCommandID( menu, count, &cmd ) );
			if ( cmd == kHICommandPreferences )
			{
				DeleteMenuItem( menu, count-- );
				
				// enable preferences item in system menu
				EnableMenuCommand( NULL, kHICommandPreferences );
				Str255 str;
				GetMenuItemText( menu, count, str );
				if ( str [0] == 1 && str [1] == '-' )
					DeleteMenuItem( menu, count );
				
  				MenuRef app_menu = NULL;
  				MenuItemIndex prefs_index = 0;
  				if ( noErr == GetIndMenuItemWithCommandID( NULL, kHICommandPreferences,
  						1, &app_menu, &prefs_index ) )
					SetMenuItemCommandKey( app_menu, prefs_index, false, ',' );
			}
		}
	}
	
	DrawMenuBar();
}

static long gestalt( OSType selector ) {
	long result = 0;
	if ( Gestalt( selector, &result ) )
		result = 0;
	return result;
}

static bool check_system_requirements()
{
	if ( gestalt( gestaltSystemVersion ) < 0x900 ) {
		report_error( "Mac OS 9 or Mac OS X is required to run this application." );
		return true;
	}
	
	if ( gestalt( gestaltCarbonVersion ) < 0x160 )
		report_error( "Your system has a version of Carbon earlier than 1.6, which "
				"this application hasn't been tested with." );
	
	return false;
}

void Carbon_App::run_app_( Carbon_App* (*create)() )
{
	if ( check_system_requirements() )
		return;
	
	try {
		unique_ptr<Carbon_App> app( create() );
		app->init_menus();
		app->init_events();
		app->run_event_loop();
	}
	catch ( ... ) {
		report_exception();
	}
}

bool Carbon_App::handle_command( const HICommandExtended& cmd )
{
	switch ( cmd.commandID )
	{
		case kHICommandAbout:
			show_about();
			return true;
		
		case kHICommandPreferences:
			edit_preferences();
			return true;
		
		case kHICommandQuit:
			if ( !request_quit() )
				return true;
			break;
		
	}
	
	return Event_Handler::handle_command( cmd );
}

bool Carbon_App::handle_event( EventRef event )
{
	unsigned long kind = GetEventKind( event );
	switch ( GetEventClass( event ) ) {
		case kEventClassApplication:
			switch ( kind ) {
				case kEventAppActivated:
					app_activated();
					break;
				
				case kEventAppDeactivated:
					app_deactivated();
					break;
			}
			break;
	}
	
	return false;
}

void Carbon_App::run_event_loop() {
	InitCursor();
	RunApplicationEventLoop();
}

void Carbon_App::opened() {
	// null
}

void Carbon_App::open_file( const FSRef& ) {
	throw_error( "This application can't open documents" );
}

void Carbon_App::open_files( const FSRef* specs, int count ) {
	while ( count-- )
		open_file( *specs++ );
}

bool Carbon_App::request_quit() {
	is_quitting_ = true;
	return true;
}

void Carbon_App::app_activated()
{
	InitCursor();
}

void Carbon_App::app_deactivated()
{
	InitCursor();
}

void Carbon_App::show_about() {
}

void Carbon_App::edit_preferences() {
}

#endif

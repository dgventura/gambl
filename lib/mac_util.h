
// Misc Mac utilities

// Game_Music_Box 0.5.2. Copyright (C) 2005 Shay Green. GNU LGPL license.

#ifndef MAC_UTIL_H
#define MAC_UTIL_H

#include "common.h"
//#include <Cocoa/Carbon.h>
//#include <Navigation.h>
//#include <InternetConfig.h>
//#include <Dialogs.h>

struct point : Point {
	point() { }
	point( int x, int y ) {
		v = y;
		h = x;
	}
	
	operator const Point* () const {
		return this;
	}
};

struct rect : Rect {
	rect() { }
	rect( int left, int top, int right, int bottom );
	
	point size() const { return point( width(), height() ); }
	int height() const { return bottom - top; }
	int width() const { return right - left; }
	
	operator const Rect* () const {
		return this;
	}
};

#ifdef GMB_COMPILE_GUI

class Event_Handler : noncopyable {
public:
	Event_Handler();
	~Event_Handler();
	
	virtual bool handle_command( const HICommandExtended& );
	virtual bool handle_event( EventRef );
	
protected:
	static EventHandlerUPP event_handler;
	void* event_handler_data() { return this; }
	
	void receive_app_events();
	
private:
	EventHandlerRef app_event_handler;
	static pascal OSStatus handle_event_( EventHandlerCallRef, EventRef, void* );
};

class Nav_Reply : public NavReplyRecord {
	mutable long size_;
public:
	Nav_Reply( NavDialogRef );
	~Nav_Reply();
	
	int size() const;
	FSRef operator [] ( int ) const;
};

class NavDialog_Holder {
	DialogRef ref;
public:
	NavDialog_Holder( DialogRef d ) : ref( d ) { }
	~NavDialog_Holder() { NavDialogDispose( ref ); }
};

struct Nav_Options : DialogCreationOptions {
	Nav_Options( const char* client_name );
};

NavEventUPP default_nav_handler();
#endif 

// Internet Config session
class Ic_Session {
	ICInstance ic;
public:
	Ic_Session();
	~Ic_Session();
	
	operator ICInstance () const { return ic; }
};

void launch_url( const char* );
OSType get_app_signature();
void install_ae_handler( AEEventID id, AEEventHandlerProcPtr proc, void* user_data = 0 );
void send_quit_event();

// True if Aqua UI is being used, which provides its own quit menu item
bool is_aqua_ui();

// True if running under OS X
bool is_os_x();

// True if application is in the foreground
bool is_front_app();

// Bring application to front
void app_to_front();

// Report currently active exception and return appropriate error code to represent it.
long report_exception( bool deferred = false );
void report_error( const char* str, bool deferred = false );

#ifdef GMB_COMPILE_GUI
void standard_alert( AlertType, const char* text );
#endif

void flush_window( WindowRef, RgnHandle = NULL );
void launch_file( const FSRef& );
void add_help_item( const char* title, long command, char key = 0 );
void open_help_document( const char* name );

int get_font_height( int id, int size, int style = 0 );

#endif


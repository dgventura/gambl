
// Simple Carbon application shell. Still evolving.

// Game_Music_Box 0.5.2. Copyright (C) 2005 Shay Green. GNU LGPL license.

#ifndef CARBON_APP_H
#define CARBON_APP_H

#include "common.h"
//#include <Files.h>
#include "mac_util.h"

class Carbon_App : noncopyable, private Event_Handler {
	static Carbon_App* app_;
	static bool is_quitting_;
public:
	// Current application object
	static Carbon_App* app() { return app_; }
	
	Carbon_App();
	virtual ~Carbon_App();
	
	// True if event loop has been/will be ended
	static bool is_quitting() { return is_quitting_; }
	
	static void run_app_( Carbon_App* (*create)() );
	
protected:
	virtual void init_menus();
	
	virtual void run_event_loop();
	
	// App opened without documents
	virtual void opened();
	
	// Documents droppped on app. Default calls open_file() for each one.
	virtual void open_files( const FSRef*, int count );
	virtual void open_file( const FSRef& );
	
	virtual bool handle_event( EventRef );
	virtual bool handle_command( const HICommandExtended& );
	
	// Application became the front-most (active) program
	virtual void app_activated();
	virtual void app_deactivated();
	
	// App has been requested to quit. Return true to allow it.
	virtual bool request_quit();
	
	// kHICommandAbout
	virtual void show_about();
	
	// kHICommandPreferences
	virtual void edit_preferences();
	
private:
	void init_events();
	static pascal OSErr ae_open_files( const AppleEvent*, AppleEvent*, long );
	static pascal OSErr ae_open( const AppleEvent*, AppleEvent*, long );
};

// Create and run application
template<class T>
class run_carbon_app {
	static Carbon_App* create() {
		return new T;
	}
public:
	run_carbon_app() {
		Carbon_App::run_app_( create );
	}
};

#endif


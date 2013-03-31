
// Game_Music_Box 0.5.2. Copyright (C) 2005 Shay Green. GNU LGPL license.

#ifndef ERRORS_WINDOW_H
#define ERRORS_WINDOW_H

#include "common.h"
//#include <Lists.h>
#include "Carbon_Window.h"
#include "mac_controls.h"

class Errors_Window : public Carbon_Window {
public:
	
	Errors_Window( const char* title, const char* text );
	~Errors_Window();
	
	void add_error( const char* );
	void add_file_error( const FSRef&, const char* );
	void add_file_exception( const FSRef& );
	
	// True if window exists, false if no errors
	bool make_window();
	
	void clear();
	
protected:
	void bounds_changed( int, int );
	bool handle_command( const HICommandExtended& );
private:
	ControlRef list_box;
	ListHandle list_ref;
	const char* title;
	const char* text;
	int old_width;
	int old_height;
	int error_count;
	
	void add_file_error_( const FSRef&, const char* );
};

inline void Errors_Window::add_file_error( const FSRef& path, const char* str ) {
	add_file_error_( path, str ? str : "" );
}

inline void Errors_Window::add_file_exception( const FSRef& path ) {
	add_file_error_( path, NULL );
}

#endif


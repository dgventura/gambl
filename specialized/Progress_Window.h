
// Game_Music_Box 0.5.2. Copyright (C) 2005 Shay Green. GNU LGPL license.

#ifndef PROGRESS_WINDOW_H
#define PROGRESS_WINDOW_H

#include "common.h"
//#include "Carbon_Window.h"
#include "mac_controls.h"

class Progress_Hook {
public:
	Progress_Hook();
	~Progress_Hook() { }
	
	void set_total( int );
	bool is_total_set() const;
	void advance( int = 1 );
	virtual void set_text( const char* );
	void update( double progress );
	virtual bool give_time();
	virtual void set_progress( double progress ) = 0;
	virtual bool has_time_passed();
	
private:
	double scale;
	int count;
};

#ifdef GMB_COMPILE_GUI

class Progress_Window : protected Carbon_Window, public Progress_Hook {
public:
	explicit Progress_Window( const char* title, bool modal = true );
	~Progress_Window();
	
	void progress_known() { } // to do: remove
	
	void set_text( const char* );
	void set_progress( double );
	
	// Show immediately, rather than waiting a bit
	void show();
	
	// True if stop button was pressed, command-period/escape was pressed,
	// or application is quitting.
	bool was_stopped() const;
	
	// Animate bar if progress is unknown
	void idle();
	
	bool give_time();
	bool has_time_passed();
	
	
	// End of public interface
	
private:
	unsigned long next_time;
	unsigned long show_time;
	Static_Text status_text;
	char text [256];
	Value_Control bar;
	ControlRef button;
	bool was_stopped_;
	bool progress_known_;
	double progress;
	bool time_passed;
	
	bool handle_command( const HICommandExtended& );
	bool handle_event( EventRef );
};

inline bool Progress_Hook::is_total_set() const {
	return scale != 0;
}
inline void Progress_Window::set_progress( double d ) {
	progress = d;
}
#endif

#endif //#ifdef GMB_COMPILE_GUI


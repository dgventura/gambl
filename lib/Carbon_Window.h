
// Game_Music_Box 0.5.2. Copyright (C) 2005 Shay Green. GNU LGPL license.

#ifdef GMB_COMPILE_GUI

#ifndef CARBON_WINDOW_H
#define CARBON_WINDOW_H

#include "common.h"
#include "mac_util.h"

class Carbon_Window : noncopyable, protected Event_Handler {
	static const Rect default_bounds;
public:
	Carbon_Window();
	~Carbon_Window();
	
	void make( WindowClass, WindowAttributes, const Rect& = default_bounds );
	
	WindowRef window() const { return window_; }
	
	void set_title( const char* );
	
	// If 'visible' is true, show window, otherwise hide it.
	void show( bool visible = true );
	
	// True if window is visible.
	bool visible() const;
	
	// Top-left position of content rectangle.
	Point top_left() const;
	
	// Restore window based on previous top_left() if it would be visible on
	// current screen configuration, otherwise leave window unchanged.
	void restore_top_left( const Point& );
	
	// WIndow was resized by user.
	virtual void bounds_changed( int width, int height );
	
protected:
	
	virtual void draw_content();
	virtual bool handle_event( EventRef );
	virtual void window_activated();
	virtual void window_deactivated();
	virtual bool handle_clickthrough( EventRef );
	
private:
	WindowRef window_;
	EventHandlerRef app_event_handler;
	bool event_handled;
};

#endif

#endif // #ifdef GMB_COMPILE_GUI
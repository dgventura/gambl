
// Channel muting window

// Game_Music_Box 0.5.2. Copyright (C) 2005 Shay Green. GNU LGPL license.

#ifndef CHANNEL_WINDOW_H
#define CHANNEL_WINDOW_H

#include "common.h"
#include "Carbon_Window.h"
#include "mac_controls.h"
class Player_Window;

class Channel_Window : public Carbon_Window {
public:
	Channel_Window( Player_Window* );
	~Channel_Window();
	
	void show( bool );
	void set_names( const char**, int count );
	void enable_eq( bool );
	
protected:
	bool handle_command( const HICommandExtended& );
private:
	enum { max_chan = 8 };
	Value_Control chan_checkboxes [max_chan];
	Value_Control eq_checkbox;
	Titled_Slider treble_slider;
	Titled_Slider bass_slider;
	Titled_Slider echo_depth;
	
	Player_Window& player;
	bool allow_custom_sound_;
	int collapsed_height;
	int expanded_height;
	const char** chan_names;
	int chan_count;
	
	void make_window();
	void update_channels();
	void update_eq_enables();
	void prefs_changed();
	static void handle_slider( void*, Slider_Control*, int );
};

#endif


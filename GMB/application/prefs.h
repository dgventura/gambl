
// Player preferences window and current values

// Game_Music_Box 0.5.2. Copyright (C) 2005 Shay Green. GNU LGPL license.

#ifndef PREFS_H
#define PREFS_H

#include "common.h"
#include "Player_Window.h"

struct App_Prefs : Player_Window::setup_t
{
	Point prefs_pos;
	bool disable_surround;
	
	// utility prefs
	struct {
		Point window_pos;
		bool remove_extensions;
		bool active;
	} util;
	
	struct {
		Point window_pos;
		bool zoomed;
		bool visible;
	} scope;
	
	Point errors_pos;
	
	bool exist;
	
	static void init();
	static void show();
	static void cleanup();
};

extern App_Prefs prefs;

#endif


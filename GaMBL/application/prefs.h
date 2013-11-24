
// Player preferences window and current values

// Game_Music_Box 0.5.2. Copyright (C) 2005 Shay Green. GNU LGPL license.

#ifndef PREFS_H
#define PREFS_H

#include "common.h"
#include "Music_Player.h"

struct App_Prefs : public Music_Player::setup_t
{
    Point player_pos;
	Point chans_pos;
	bool songs_only;
	bool show_info;
	bool shuffle;
    bool change_icon;
	
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

class Progress_Hook
{
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

#endif

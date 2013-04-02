
// Game_Music_Box 0.5.2. Copyright (C) 2005 Shay Green. GNU LGPL license.

#ifndef PLAYER_WINDOW_H
#define PLAYER_WINDOW_H

#ifdef COMPILE_GMB_GUI

#include "common.h"
#include "Carbon_Window.h"
#include "mac_controls.h"
#include "Drag_Handler.h"
#include "Music_Player.h"
#include "Channel_Window.h"
#include "wave_export.h"
#include "Music_Album.h"
class Cat_Info;
struct Player_Scope;

class Player_Window : public Carbon_Window, private Drag_Handler {
public:
	Player_Window();
	~Player_Window();
	
	struct setup_t : Music_Player::setup_t {
		Point player_pos;
		Point chans_pos;
		bool songs_only;
		bool show_info;
		bool shuffle;
		bool change_icon;
	};
	
	void setup_changed();
	
	void begin_drop();
	void handle_drop( const FSRef& );
	
	// Returns true if new tracks were added
	bool end_drop( bool immediate = false );
	
	void set_mute( int mask );
	
	void uncache();
	
	const track_ref_t& track() const;
	const Music_Album* album() const;
	
private:
	// interface
	Control prev_button;
	Control pause_button;
	Control next_button;
	Slider_Control volume_slider;
	Channel_Window channels_window;
	MenuRef dock_menu;
	int collapsed_height;
	int expanded_height;
	bool recording;
	bool auto_unpause;
	void show_info( bool );
	bool handle_command( const HICommandExtended& );
	bool handle_event( EventRef );
	void enable_buttons();
	void schedule_auto_unpause();
	void choose_music();
	bool handle_clickthrough( EventRef );
	static void handle_slider( void*, Slider_Control*, int );
	void adjust_volume( int delta );
	void playing_changed();
	void make_window();
	void track_changed();
	void stopped();
	
	BOOST::scoped_ptr<Player_Scope> scope;
	void show_scope();
	
	// incoming files
	Music_Queue new_queue_;
	void queue_files( const FSRef&, const Cat_Info&, HFSUniStr255&,
			Music_Queue&, int depth );
	bool handle_drag( const FSRef&, OSType, bool );
	void items_dropped( DragReference );
	
	// queue
	Music_Queue queue;
	Music_Queue history;
	int history_pos;
	track_ref_t& current();
	bool has_future();
	bool prev_file( bool last_track = false );
	bool play_current();
	void open_error();
	
	// player
	BOOST::scoped_ptr<Music_Album> music_album;
	Music_Player player;
	FSRef album_path;
	int mute_mask;
	bool playing;
	bool play_current_();
	bool start_track();
	bool prev_track();
	bool next_track();
	void toggle_pause();
	void stop( bool clear_history = false );
	
	// fast-forward
	bool fast_forwarding;
	bool enable_fast_forward( bool );
	static pascal void button_action( ControlRef, ControlPartCode );
	
	// info text
	Static_Text system_text;
	Static_Text track_text;
	Static_Text time_text;
	Static_Text author_text;
	Static_Text copyright_text;
	FSRef proxy_path;
	void update_info();
	void update_time();
	void update_track();
	
	// periodic checking
	Event_Loop_Timer track_timer;
	void install_track_timer( bool delay );
	void check_track_end();
	static void check_track_end_( void* );
};

inline void Player_Window::set_mute( int mask ) {
	mute_mask = mask;
	player.set_mute( mask );
}
inline const track_ref_t& Player_Window::track() const {
	return const_cast<Player_Window*> (this)->current();
}
inline const Music_Album* Player_Window::album() const {
	return music_album.get();
}

#else
#include "Music_Player.h"
class Player_Window {
public:
	Player_Window();
	~Player_Window();
	
	struct setup_t : Music_Player::setup_t {
		Point player_pos;
		Point chans_pos;
		bool songs_only;
		bool show_info;
		bool shuffle;
		bool change_icon;
	};
};

#endif

#endif // COMPILE_GMB_GUI


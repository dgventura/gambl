
// Window to select and execute actions to take on items

// Game_Music_Box 0.5.2. Copyright (C) 2005 Shay Green. GNU LGPL license.

#ifndef UTILITY_WINDOW_H
#define UTILITY_WINDOW_H

#include "common.h"
#include "pod_vector.h"
#include "Carbon_Window.h"
#include "mac_controls.h"
#include "Drag_Handler.h"
#include "Errors_Window.h"
#include "File_Emu.h"
class Player_Window;

class Utility_Window : public Carbon_Window, private Drag_Handler {
public:
	Utility_Window( Player_Window* );
	~Utility_Window();
	
	void cancel();
	
private:
	typedef pod_vector<FSRef> items_t;
	items_t items;
	ControlRef list_box;
	ListHandle list_ref;
	Player_Window* player_window;
	
	Value_Control reassociate_radio;
		Value_Control remove_ext_box;
	Value_Control retitle_radio;
	Value_Control compress_radio;
		Value_Control pack_spcs_box;
	Value_Control expand_radio;
	Value_Control scan_radio;
	Value_Control record_radio;
	int recording_pos;
	bool was_stopped;
	bool processing;
	File_Emu::setup_t recording_setup;
	
	Static_Text progress_text;
	Value_Control progress_bar;
	
	ControlRef add_button;
	ControlRef clear_button;
	ControlRef action_button;
	bool pack_spcs;
	
	Errors_Window errors_window;
	
	void items_dropped( DragReference );
	bool handle_drag( const FSRef&, OSType, bool );
	void add_item( const FSRef& );
	void process_files();
	
	bool handle_command( const HICommandExtended& );
	bool handle_event( EventRef );
	void reflect_controls( bool set = false );
	void make_window();
	void clear_items();
	void set_action( long command_id );
	void choose_items();
	void enable_controls( bool );
	
	friend class Utility_Progress;
};

#endif


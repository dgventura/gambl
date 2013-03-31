
// Game_Music_Box 0.5.2. http://www.slack.net/~ant/game-music-box

#include "Utility_Window.h"

//#include <ControlDefinitions.h>

#include "Progress_Window.h"
#include "music_actions.h"
#include "Errors_Window.h"
#include "wave_export.h"
#include "Carbon_App.h"
#include "file_util.h"
#include "prefs.h"

/* Copyright (C) 2005 Shay Green. This module is free software; you
can redistribute it and/or modify it under the terms of the GNU Lesser
General Public License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version. This
library is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for
more details. You should have received a copy of the GNU Lesser General
Public License along with this module; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA */

#include "source_begin.h"

// to do: re-enable SPC packing?
//pack_spcs_box

const int window_width = 450;

static rect indent( const rect& in, int offset = 30 ) {
	rect r = in;
	r.left += 30;
	r.right += 30;
	return r;
}

Utility_Window::Utility_Window( Player_Window* pw ) :
	errors_window( "Utility Errors", 
			"Problems were encountered with the following items:" ),
	player_window( pw )
{
	pack_spcs = false;
	
	make_window();
	
	clear_items();
	set_action( 'RAss' );
	reflect_controls( true );
	
	Drag_Handler::init( window() );
	
	Carbon_Window::restore_top_left( prefs.util.window_pos );
}

Utility_Window::~Utility_Window()
{
	prefs.util.window_pos = Carbon_Window::top_left();
	if ( errors_window.window() )
		prefs.errors_pos = errors_window.top_left();
}

void Utility_Window::make_window()
{
	Carbon_Window::make( kDocumentWindowClass,
			kWindowStandardHandlerAttribute | kWindowCloseBoxAttribute );
	set_title( "Game Music Utility" );
	
	// Required on OS 9 or list box scrollbars won't work
	ControlRef root;
	CreateRootControl( window(), &root );
	
	const int right_width = window_width / 2;
	
	Row_Placer left_rows( window_width / 2 );
	Row_Placer right_rows( right_width, point( left_rows.bounds().right, 4 ) );
	left_rows.set_spacing( 0 );
	right_rows.set_spacing( 4 );
	
	// Help text
	{
		const char* info_text = "Add or drag music files, archives, and folders to the list, "
				"select an action, then click Start.";
		rect r = left_rows.place( 40 );
		r.right = right_rows.place( 40 ).right;
		make_static_text( window(), r, info_text );
	}
	
	// Action radio buttons and options
	
	{
		const int button_height = 16;
		const int button_width = 200;
		
		reassociate_radio.make_radio( window(), right_rows.place( button_height, button_width ),
				"Update Icons", 'RAss' );
		
			remove_ext_box.make_checkbox( window(),
					indent( right_rows.place( button_height, button_width ) ),
					(is_os_x() ? "Hide File Extensions" : "Remove File Extensions"), 'Updt' );
		
		retitle_radio.make_radio( window(), right_rows.place( button_height, button_width ),
				"Rename Using Info Tags", 'RTit' );
		set_control_help( retitle_radio,
				"Renames files and archives based on information tags (if present)" );
		
		compress_radio.make_radio( window(), right_rows.place( button_height, button_width ),
				"Compress Files", 'RCmp' );
		set_control_help( compress_radio,
				"Compresses individual files with gzip" );
			
			//pack_spcs_box.make_checkbox( window(), indent( indent( right_rows.place( button_height, button_width ) ) ),
			//      "Pack SPC Files", 'Updt' );
			//set_control_help( pack_spcs_box,
			//      "Compresses SPC music much more than gzip, but isn't widely supported" );
		
		expand_radio.make_radio( window(), right_rows.place( button_height, button_width ),
				"Expand Files", 'RExp' );
		set_control_help( expand_radio,
				"Expands individual files only (not archives)" );
		
		scan_radio.make_radio( window(), right_rows.place( button_height, button_width ),
				"Check For Problems", 'RChk' );
		set_control_help( scan_radio,
				"Checks integrity and compatibility of files and archives" );
		
		record_radio.make_radio( window(), right_rows.place( button_height, button_width ),
				"Record To Folder", 'RRec' );
	}
	
	right_rows.place( 8 );
	
	// progress
	
	const int start_width = 80;
	ControlFontStyleRec style;
	style.flags = kControlUseFaceMask;
	style.style = condense;
	progress_text.make( window(), right_rows.place( 16, right_width - start_width ), style );
	progress_text.enable_truncation();
	int list_bottom = right_rows.bounds().bottom;
	
	{
		Row_Filler row( right_rows.place( 20 ) );
		
		row.place_right( 2 );
		action_button = make_push_button( window(), row.place_right( start_width - 6 ),
				"Start", 'BPro' );
		make_default_button( action_button );
		row.place_right( 4 );
		
		ControlRef control;
		rect r = row.remain();
		r.top += 4;
		debug_if_error( CreateProgressBarControl( window(), r,
				0, 0, 0x1000, true, &control ) );
		progress_bar.reset( control );
	}
	
	right_rows.place( 0 );
	
	// item list
	
	static const ListDefSpec spec = { kListDefStandardTextType };
	rect r = left_rows.place( list_bottom - left_rows.bounds().bottom );
	r.top += 4;
	r.right -= 15;
	r.bottom -= 4;
	r.left += 4;
	throw_if_error( CreateListBoxControl( window(), r,
			false, 0, 1, false, true, 16, window_width, false, &spec, &list_box ) );
	
	throw_if_error( GetControlData( list_box, kControlListBoxPart,
			kControlListBoxListHandleTag, sizeof list_ref, &list_ref, NULL ) );
	SetListSelectionFlags( list_ref, lNoNilHilite | lOnlyOne );
	left_rows.place( 4 );
	
	// list actions
	
	{
		Row_Filler row( left_rows.place( 20 ) );
		
		row.place_left( 20 );
		clear_button = make_push_button( window(), row.place_left( 60 ), "Clear", 'BClr' );
		
		row.place_right( 40 );
		add_button = make_push_button( window(), row.place_right( 60 ), "Add...", 'BAdd' );
	}
	
	// window bounds
	SizeWindow( window(), right_rows.bounds().right + 4, right_rows.bounds().bottom + 4, true );
	RepositionWindow( window(), NULL, kWindowAlertPositionOnMainScreen );
}

static void reflect_control_bool( bool set, bool& value, Value_Control& control )
{
	if ( set )
		control.set_value( value );
	else
		value = control.value();
}
 
void Utility_Window::reflect_controls( bool set )
{
	reflect_control_bool( set, prefs.util.remove_extensions, remove_ext_box );
	//reflect_control_bool( set, prefs.util.pack_spcs, pack_spcs_box );
	
	remove_ext_box.enable( reassociate_radio.value() );
}

void Utility_Window::enable_controls( bool b )
{
	enable_control( retitle_radio, b );
	enable_control( compress_radio, b );
	enable_control( expand_radio, b );
	enable_control( scan_radio, b );
	enable_control( record_radio, b );
	enable_control( clear_button, b );
	enable_control( add_button, b );
	enable_control( list_box, b );
	enable_control( reassociate_radio, b );
	enable_control( remove_ext_box, b );
	if ( b )
		reflect_controls();
}

void Utility_Window::clear_items()
{
	processing = false;
	
	items.clear();
	pod_vector<FSRef> empty;
	items.swap( empty );
	
	enable_controls( true );
	set_control_title( action_button, "Start" );
	enable_control( action_button, false );
	recording_pos = 0;
	progress_text.set_text( "" );
	HideControl( progress_bar );
	
	rect r;
	GetListDataBounds( list_ref, &r );
	if ( r.height() )
	{
		LDelRow( r.height(), 0, list_ref );
		Draw1Control( list_box );
	}
}

void Utility_Window::set_action( long which )
{
	reassociate_radio.set_value( which == 'RAss' );
	retitle_radio.set_value( which == 'RTit' );
	compress_radio.set_value( which == 'RCmp' );
	expand_radio.set_value( which == 'RExp' );
	scan_radio.set_value( which == 'RChk' );
	record_radio.set_value( which == 'RRec' );
	if ( !record_radio.value() )
	{
		if ( recording_pos )
			set_control_title( action_button, "Start" );
		recording_pos = 0;
	}
	
	reflect_controls();
	progress_text.set_text( "" );
}

bool Utility_Window::handle_command( const HICommandExtended& cmd )
{
	switch ( cmd.commandID )
	{
		case 'Clos':
			// for some reason this is ignored after window is hidden
			enable_control( action_button, false );
			show( false );
			clear_items();
			set_action( 'RAss' );
			was_stopped = true;
			return true;
		
		case 'BClr':
			clear_items();
			return true;
		
		case 'BAdd':
			choose_items();
			return true;
		
		case 'Updt':
			reflect_controls();
			return true;
		
		case 'BPro':
			was_stopped = true;
			if ( !processing )
				process_files();
			return true;
		
		case 'RAss':
		case 'RTit':
		case 'RCmp':
		case 'RExp':
		case 'RChk':
		case 'RRec':
			set_action( cmd.commandID );
			return true;
	}
	
	return Carbon_Window::handle_command( cmd );
}

bool Utility_Window::handle_event( EventRef event )
{
	UInt32 kind = GetEventKind( event );
	switch ( GetEventClass( event ) )
	{
		case kEventClassKeyboard:
			if ( processing && IsUserCancelEventRef( event ) )
			{
				press_button( action_button );
				was_stopped = true;
				return true;
			}
			
			if ( kind == kEventRawKeyDown )
			{
				UInt32 code = 0;
				debug_if_error( GetEventParameter( event, kEventParamKeyCode, typeUInt32,
						NULL, sizeof code, NULL, &code ) );
				
				if ( (code == 36 || code == 76) && !processing && !items.empty() )
				{
					press_button( action_button );
					process_files();
					return true;
				}
			}
			break;
	}
	
	return Carbon_Window::handle_event( event );
}

void Utility_Window::items_dropped( DragReference drag )
{
	Drag_Handler::items_dropped( drag );
	Draw1Control( list_box );
}

bool Utility_Window::handle_drag( const FSRef& spec, OSType type, bool take_action )
{
	if ( processing )
		return false;
	
	if ( !is_dir_type( type ) && !identify_music_file( spec, type ) &&
			!identify_music_file_data( FSResolveAliasFileChk( spec ) ) )
		return false;
	
	if ( take_action )
		add_item( spec );
	
	return true;
}

void Utility_Window::choose_items()
{
	// run dialog
	Nav_Options options( PROGRAM_NAME );
	options.windowTitle = CFSTR( "Add Items" );
	options.message = CFSTR( "Choose music files and folders to add" );
	options.actionButtonLabel = CFSTR( "Add" );
	options.optionFlags |= kNavAllowMultipleFiles;
	NavDialogRef dialog;
	throw_if_error( NavCreateChooseObjectDialog( &options, default_nav_handler(),
			NULL, NULL, NULL, &dialog ) );
	NavDialog_Holder holder( dialog );
	throw_if_error( NavDialogRun( dialog ) );
	
	// get reply
	Nav_Reply reply( dialog );
	if ( reply.validRecord )
	{
		for ( int i = 0; i < reply.size(); i++ )
			add_item( reply [i] );
		Draw1Control( list_box );
	}
}

void Utility_Window::add_item( const FSRef& path_in )
{
	int index = items.size();
	
	FSRef path = FSResolveAliasFileChk( path_in );
	items.push_back( path );
	enable_control( action_button, true );
	progress_text.set_text( "" );
	
	char filename [256];
	get_filename( path, filename, sizeof filename );
	LAddRow( 1, index, list_ref );
	LSetCell( filename, std::strlen( filename ), point( 0, index ), list_ref );
}

class Utility_Progress : public Action_Hooks, public Progress_Hook {
public:
	Utility_Progress( Utility_Window* );
	~Utility_Progress();
	void files_counted();
	bool give_time();
	bool advance( const FSRef& path, int count );
	void log_error( const FSRef& path, const char* error );
	void log_exception( const FSRef& path );
	bool is_scanning() const { return file_count >= 0; }
	void set_final_text( const char* str )
	{
		final_text = str;
	}
	
	void set_text( const char* str )
	{
		std::strncpy( text, str, sizeof text );
		text [(sizeof text - 1)] = 0;
	}
	
	bool has_time_passed()
	{
		bool result = time_passed;
		time_passed = false;
		return result;
	}
	
	void set_progress( double p )
	{
		progress = p;
	}
	
	bool was_stopped() const;

private:
	Utility_Window& window;
	unsigned long next_time;
	double progress;
	int file_count;
	bool time_passed;
	char text [256];
	const char* final_text;
	
	void mode_changed();
};

Utility_Progress::Utility_Progress( Utility_Window* w ) : window( *w )
{
	file_count = 0;
	*text = 0;
	set_text( "Scanning..." );
	time_passed = false;
	next_time = 0;
	progress = 0;
	final_text = "";
	
	make_default_button( window.action_button, false );
	set_control_title( window.action_button, "Stop" );
	mode_changed();
	window.processing = true;
	prefs.util.active = true;
	if ( window.player_window )
		window.player_window->uncache();
	ShowControl( window.progress_bar );
	window.enable_controls( false );
}

Utility_Progress::~Utility_Progress()
{
	prefs.util.active = false;
	window.processing = false;
	
	make_default_button( window.action_button, true );
	set_control_title( window.action_button, (window.recording_pos ? "Resume" : "Start") );
	HideControl( window.progress_bar );
	window.progress_text.set_text( final_text );
	window.enable_controls( true );
}

bool Utility_Progress::was_stopped() const
{
	return window.was_stopped || Carbon_App::is_quitting();
}

void Utility_Progress::mode_changed()
{
	Boolean b = (file_count >= 0);
	debug_if_error( SetControlData( window.progress_bar, kControlIndicatorPart,
			kControlProgressBarIndeterminateTag, sizeof b, &b ) );
}

void Utility_Progress::files_counted()
{
	set_total( file_count );
	file_count = -1;
	mode_changed();
}

static void give_time()
{
	for ( int n = 12; n--; )
	{
		EventRef event;
		OSStatus result = ReceiveNextEvent( 0, NULL, kEventDurationSecond / 100, true, &event );
		if ( result == eventLoopTimedOutErr )
			break;
		if ( !debug_if_error( result ) ) {
			SendEventToEventTarget( event, GetEventDispatcherTarget() );
			ReleaseEvent( event );
		}
	}
}

bool Utility_Progress::give_time()
{
	unsigned long time = TickCount();
	if ( time >= next_time )
	{
		next_time = time + 10;
		time_passed = true;
		
		if ( *text ) {
			window.progress_text.set_text( text );
			*text = 0;
		}
		
		double p = progress;
		if ( p > 1.0 )
			p = 1.0;
		
		window.progress_bar.set_value( p * 0x1000 );
		
		IdleControls( window.window() );
		::give_time();
	}
	return was_stopped();
}

bool Utility_Progress::advance( const FSRef& path, int count )
{
	if ( file_count >= 0 )
	{
		file_count += count;
		return false;
	}
	
	Progress_Hook::advance( count );
	if ( has_time_passed() )
	{
		char name [256];
		filename_without_extension( path, name );
		set_text( name );
	}
	
	return !give_time();
}

void Utility_Progress::log_error( const FSRef& path, const char* str )
{
	require( str );
	if ( !is_scanning() )
		window.errors_window.add_file_error( path, str );
}

void Utility_Progress::log_exception( const FSRef& path )
{
	if ( !is_scanning() )
		window.errors_window.add_file_exception( path );
}

void Utility_Window::process_files()
{
	if ( items.empty() )
		return;
	
	errors_window.clear();
	was_stopped = false;
	
	Utility_Progress pw( this );
	
	if ( record_radio.value() )
	{
		
		// get location
		FSRef out_dir;
		if ( choose_folder( &out_dir ) )
		{
			pw.set_final_text( "Recording suspended." );
			
			Music_Queue queue;
			for ( int i = 0; i < items.size(); i++ )
				append_playlist( items [i], queue );
			
			if ( recording_pos >= queue.size() )
				recording_pos = 0;
			
			if ( recording_pos == 0 )
				recording_setup = prefs;
			
			// to do: way to advance without file ref
			pw.advance( items [0], queue.size() );
			pw.files_counted();
			pw.advance( items [0], recording_pos );
			
			for ( ; recording_pos < queue.size(); recording_pos++ )
			{
				try
				{
					record_track_( queue [recording_pos], out_dir, recording_setup, pw );
				}
				catch ( ... )
				{
					handle_disk_full();
					errors_window.add_file_exception( queue [recording_pos] );
				}
				
				pw.Progress_Hook::advance();
				
				if ( pw.give_time() )
					break;
			}
			
			if ( recording_pos == queue.size() )
			{
				recording_pos = 0; // can't resume
				pw.set_final_text( "Recording complete." );
			}
		}
	}
	else
	{
		const char* final_text = "";
		
		for ( int phase = 2; phase--; )
		{
			for ( int i = 0; i < items.size(); i++ )
			{
				try
				{
					if ( reassociate_radio.value() ) {
						associate_music( items [i], pw, remove_ext_box.value() );
						final_text = "Icon update complete.";
					}
					else if ( retitle_radio.value() ) {
						retitle_music( items [i], pw );
						final_text = "Renaming complete.";
					}
					else if ( compress_radio.value() ) {
						compress_music( items [i], pw, pack_spcs );
						final_text = "Compression complete.";
					}
					else if ( expand_radio.value() ) {
						expand_music( items [i], pw );
						final_text = "Expansion complete.";
					}
					else if ( scan_radio.value() ) {
						check_music_files( items [i], pw );
						final_text = "Checking complete.";
					}
				}
				catch ( ... ) {
					errors_window.add_file_exception( items [i] );
				}
				
				if ( pw.give_time() )
					goto stopped;
			}
			
			if ( phase )
				pw.files_counted();
		}
		
		pw.set_final_text( final_text );
	}
	
stopped:
	if ( errors_window.make_window() )
	{
		if ( prefs.errors_pos.h | prefs.errors_pos.v ) {
			errors_window.restore_top_left( prefs.errors_pos );
			prefs.errors_pos = point( 0, 0 );
		}
		
		if ( !errors_window.visible() ) {
			errors_window.show();
			SelectWindow( errors_window.window() );
		}
	}
}


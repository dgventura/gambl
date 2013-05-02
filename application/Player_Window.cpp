
// Game_Music_Box 0.5.2. http://www.slack.net/~ant/game-music-box

#ifdef COMPILE_GMB_GUI

#include "Player_Window.h"

//#include <ControlDefinitions.h>
//#include <MacApplication.h>
//#include <Folders.h>

#include "prefs.h"
#include "wave_export.h"
#include "file_util.h"
#include "music_util.h"
#include "Scope_Window.h"

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

const int history_max = 2000; // history is trimmed if it goes above this size
const int window_width = 300;

struct Player_Scope : Scope_Window
{
	Music_Player* player;
	const void* last_buf;
	
	Player_Scope()
	{
		player = NULL;
		last_buf = NULL;
	}
	
	void clear( bool flatline = false )
	{
		last_buf = NULL;
		Scope_Window::clear( flatline );
	}
	
	const short* get_buffer( bool update_only )
	{
		if ( !player )
			return NULL;
		
		const short* p = player->scope_buffer();
		if ( !update_only && p == last_buf )
			return NULL;
		
		last_buf = p;
		return p;
	}
};

static void check_menu( long cmd, bool b ) {
	SetMenuCommandMark( NULL, cmd, (b ? checkMark : noMark) );
}

static void set_shuffle( bool b ) {
	prefs.shuffle = b;
	check_menu( 'Shuf', b );
}

static void set_skip_tracks( bool b ) {
	prefs.songs_only = b;
	check_menu( 'SkpS', b );
}

static void set_length( int len )
{
	for ( int i = 0; i < 4; i++ )
		check_menu( 'Len0' + i, len == i );
	static float durations [] = { 1.0 / 2.73, 1.0, 2.0564, 100 };
	prefs.duration = durations [len];
}

Player_Window::Player_Window() :
	channels_window( this )
{
	mute_mask = 0;
	playing = false;
	dock_menu = NULL;
	history_pos = -1;
	recording = false;
	
	make_window();
	
	Drag_Handler::init( window() );
	
	receive_app_events();
	
	track_timer.set_callback( check_track_end_, this );
	
	SetWindowModified( window(), false ); // enables proxy icon and popup
	
	// window size and location
	show_info( prefs.show_info );
	RepositionWindow( window(), NULL, kWindowAlertPositionOnMainScreen );
	restore_top_left( prefs.player_pos );
	
	stop();
	
	set_length( 1 );
	set_skip_tracks( prefs.songs_only );
	set_shuffle( prefs.shuffle );
	setup_changed();
	
	show( true );
	
	// show channels window on first launch
	if ( !prefs.exist )
		channels_window.show( true );
	
	if ( prefs.scope.visible )
		show_scope();
	
	dock_menu = NULL;
	if ( is_os_x() )
	{
		dock_menu = GetMenu( 200 );
		check( dock_menu );
		if ( dock_menu )
			debug_if_error( SetApplicationDockTileMenu( dock_menu ) );
	}
}

Player_Window::~Player_Window()
{
	prefs.player_pos = Carbon_Window::top_left();
}

void Player_Window::make_window()
{
	Carbon_Window::make( kDocumentWindowClass, kWindowCloseBoxAttribute |
			kWindowStandardHandlerAttribute | kWindowVerticalZoomAttribute |
			kWindowNoActivatesAttribute | (is_os_x() ? kWindowCollapseBoxAttribute : 0) );
	
	// remove from window menu
	//ChangeWindowAttributes( window(), 0, kWindowInWindowMenuAttribute );
	
	const int button_width = 32;
	const int first_column = button_width + 4 + button_width + 4 + button_width;
	
	Row_Placer rows( first_column );
	Row_Placer info_rows( window_width - first_column - 12, point( first_column + 8, 4 ) );
	info_rows.set_spacing( 0 );
	rows.set_spacing( 0 );
	
	// track control
	{
		Row_Filler row( rows.place( 20 ), 4 );
		
		prev_button.reset( make_cicon_button( window(),
				row.place_left( button_width ), 130, 'Prev' ) );
		
		pause_button.reset( make_cicon_button( window(),
				row.place_left( button_width ), 128, 'Paus' ) );
		
		next_button.reset( make_cicon_button( window(),
				row.place_left( button_width ), 131, 'Next' ) );
		SetControlReference( next_button, (SInt32) this );
		SetControlAction( next_button, NewControlActionUPP( button_action ) );
	}
	rows.place( 4 );
	
	{
		Row_Filler row( rows.place( 16 ), 0 );
		
		make_static_icon( window(), row.place_left( 8 ), 134 );
		row.place_left( 1 );
		make_static_icon( window(), row.place_right( 12 ), 135 );
		row.place_right( 2 );
		
		volume_slider.make( window(), row.remain(), 0, handle_slider, this );
		volume_slider.set_fraction( prefs.volume );
	}
	rows.place( 4 );
	
	ControlFontStyleRec style;
	style.flags = kControlUseFontMask | kControlUseFaceMask | kControlUseSizeMask;
	style.font = kFontIDGeneva;
	style.style = normal;
	style.size = 12;
	
	int text_height = get_font_height( style.font, style.size, style.style );
	int padding = rows.bounds().height() - text_height * 2;
	
	info_rows.place( 4 );
	track_text.make( window(), info_rows.place( text_height * 2 ), style );
	info_rows.place( padding - 4 );
	
	collapsed_height = std::max( rows.bounds().bottom, info_rows.bounds().bottom );
	
	time_text.make( window(), rows.place( text_height ), style );
	
	system_text.make( window(), rows.place( text_height ), style );
	
	author_text.make( window(), info_rows.place( text_height ), style );
	author_text.enable_truncation();
	
	copyright_text.make( window(), info_rows.place( text_height ), style );
	copyright_text.enable_truncation();
	
	expanded_height = std::max( rows.bounds().bottom, info_rows.bounds().bottom ) + 4;
}

void Player_Window::show_info( bool b )
{
	prefs.show_info = b;
	update_time();
	SizeWindow( window(), window_width, (b ? expanded_height : collapsed_height), true );
}

void Player_Window::install_track_timer( bool delay )
{
	track_timer.install( fast_forwarding ? 0.1 : 0.5,
			!fast_forwarding && delay ? 1.0 : 0 );
}

static void append_time( char* str, int seconds )
{
	char num [32];
	num_to_str( seconds / 60, num );
	std::strcat( str, num );
	std::strcat( str, ":" );
	num_to_str( seconds % 60, num, -2 );
	std::strcat( str, num );
}

void Player_Window::update_time()
{
	if ( !prefs.show_info || !music_album )
		return;
	
	char str [64];
	
	str [0] = 0;
	append_time( str, player.elapsed() );
	
	int duration = player.track_length();
	if ( duration < 60 * 60 ) {
		std::strcat( str, " / " );
		append_time( str, duration );
	}
	
	time_text.set_text( str );
}

static int get_bundle_volume()
{
	Cat_Info info;
	info.volume = kOnSystemDisk;
	try {
		info.read( get_bundle_fsref(), kFSCatInfoVolume );
	}
	catch ( ... ) {
		check( false );
	}
	return info.volume;
}

void Player_Window::update_info()
{
	set_title( music_album->info().game );
	
	if ( 0 != std::memcmp( &proxy_path, static_cast<FSRef*> (&current()), sizeof proxy_path ) )
	{
		proxy_path = current();
		FSSpec spec;
		if ( !FSGetCatalogInfo( &current(), 0, NULL, NULL, &spec, NULL ) )
		{
			static int volume = get_bundle_volume();
			debug_if_error( SetWindowProxyFSSpec( window(),  &spec ) );
			
			// to do: proxy icon often gets corrupt, even in Finder. Is setting
			// creator and type triggering a bug in the system?
			// to do: Specify icon directly (using IconRef)?
			debug_if_error( SetWindowProxyCreatorAndType( window(), gmb_creator,
					music_album->music_type(), volume ) );
		}
		else {
			check( false );
		}
	}
	
	const Music_Album::info_t& info = music_album->info();
	system_text.set_text( info.system );
	
	ControlFontStyleRec style;
	style.flags = kControlUseFontMask | kControlUseFaceMask | kControlUseSizeMask;
	style.font = kFontIDGeneva;
	style.style = normal;
	style.size = 12;
	
	char str [256];
	
	// to do: find actual width of string when selecting condensed or normal
	
	str [0] = 0;
	if ( *info.author ) {
		std::strcat( str, "By " );
		std::strcat( str, info.author );
	}
	// Field can only hold 16 "W" characters (at the moment), but most strings
	// use narrower characters, so on average 26 fit.
	style.style = (std::strlen( str ) < 26) ? normal : condense;
	debug_if_error( SetControlFontStyle( author_text, &style ) );
	author_text.set_text( str );
	
	str [0] = 0;
	if ( *info.copyright ) {
		std::strcat( str, "(C) " );
		std::strcat( str, info.copyright );
	}
	style.style = (std::strlen( str ) < 26) ? normal : condense;
	debug_if_error( SetControlFontStyle( copyright_text, &style ) );
	copyright_text.set_text( str );
}

void Player_Window::update_track()
{
	if ( !music_album )
		return;
	
	char str [256];
	str [0] = 0;
	
	int track = current().track;
	const char* name = music_album->info().song;
	
	int track_count = music_album->track_count();
	if ( track_count > 1 && !prefs.shuffle )
	{
		num_to_str( track + 1, str_end( str ) );
		std::strcat( str, "/" );
		num_to_str( track_count, str_end( str ) );
		std::strcat( str, ": " );
	}
	
	char* past_num = str_end( str );
	
	if ( *name ) {
		std::strcat( str, name );
	}
	else {
		std::strcat( str, "Track " );
		num_to_str( track + 1, str_end( str ) );
	}
	
	track_text.set_text( str );
	
	std::strcpy( past_num + 48, "..." );
	if ( dock_menu )
	{
		Str255 ps;
		c2pstrcpy( ps, past_num );
		SetMenuItemText( dock_menu, 1, ps );
	}
}

void Player_Window::track_changed()
{
	// update window ui
	update_track();
	update_info();
	prev_button.enable( true );
	next_button.enable( true );
	flush_window( window() ); // update UI as quickly as possible
	
	// might have been hidden
	if ( !visible() ) {
		show( true );
		SelectWindow( window() );
	}
	
	install_track_timer( true );
	
	// update other ui
	channels_window.enable_eq( music_album->info().classic_emu );
	channels_window.set_names( player.music_emu()->voice_names(), player.music_emu()->voice_count() );
	EnableMenuCommand( NULL, 'Expo' );
	if ( music_album->info().duration < 0 )
		DisableMenuCommand( NULL, 'ECur' );
	else
		EnableMenuCommand( NULL, 'ECur' );
	
	if ( recording || !is_front_app() || prefs.shuffle || prefs.util.active )
		uncache();
}

void Player_Window::stopped()
{
	enable_fast_forward( false );
	playing = false;
	playing_changed();
	prev_button.enable( history_pos > 0 );
	next_button.enable( history_pos + 1 < history.size() );
	RemoveWindowProxy( window() );
	std::memset( &proxy_path, 0, sizeof proxy_path );
	set_title( PROGRAM_NAME );
	track_text.set_text( "Drop game music here" );
	time_text.set_text( "" );
	author_text.set_text( "" );
	copyright_text.set_text( "" );
	system_text.set_text( "" );
	
	flush_window( window() ); // update UI as quickly as possible
	
	if ( scope )
		scope->clear();
	
	channels_window.enable_eq( true );
	channels_window.set_names( NULL, 0 );
	DisableMenuCommand( NULL, 'Expo' );
	DisableMenuCommand( NULL, 'ECur' );
	if ( dock_menu )
		SetMenuItemText( dock_menu, 1, "\pNot Playing" );
	
	track_timer.remove();
	reset_container( new_queue_ );
	fast_forwarding = false;
	auto_unpause = false;
}

void Player_Window::setup_changed()
{
	player.setup_changed( prefs );
	update_time(); // play length may have changed
	update_track(); // shuffle may have changed
}

void Player_Window::playing_changed()
{
	set_button_icon( pause_button, 128 + playing );
	if ( dock_menu )
		SetMenuItemText( dock_menu, 4, playing ? "\pPause" : "\pPlay" );
}

void Player_Window::toggle_pause()
{
	playing = !playing;
	playing_changed();
	if ( playing ) {
		player.resume();
	}
	else {
		player.pause();
		update_time();
	}
}

bool Player_Window::enable_fast_forward( bool b )
{
	bool result = false;
	if ( fast_forwarding != b )
	{
		fast_forwarding = b;
		result = player.enable_fast_forward( b );
		install_track_timer( false );
		update_time();
	}
	return result;
}

pascal void Player_Window::button_action( ControlRef control, ControlPartCode part ) {
	Player_Window* self = reinterpret_cast<Player_Window*> (GetControlReference( control ));
	self->enable_fast_forward( part == kControlIconPart && self->music_album && self->playing );
}

void Player_Window::check_track_end()
{
	if ( auto_unpause ) {
		auto_unpause = false;
		if ( playing )
			player.resume();
	}
	
	if ( fast_forwarding | playing )
	{
		update_time();
		
		if ( player.is_done() )
		{
			try {
				next_track();
			}
			catch ( ... ) {
				report_exception();
			}
		}
	}
}

void Player_Window::check_track_end( void* self_ ) {
	static_cast<Player_Window*> (self_)->check_track_end();
}

void Player_Window::handle_slider( void* self_, Slider_Control*, int )
{
	Player_Window* self = static_cast<Player_Window*> (self_);
	prefs.volume = self->volume_slider.fraction();
	self->player.setup_changed( prefs );
}

void Player_Window::adjust_volume( int delta )
{
	double v = volume_slider.fraction();
	v += delta * 0.05;
	if ( v < 0 )
		v = 0;
	if ( v > 1 )
		v = 1;
	prefs.volume = v;
	volume_slider.set_fraction( v );
	player.setup_changed( prefs );
}

bool Player_Window::handle_clickthrough( EventRef event_in )
{
	if ( is_os_x() )
	{
		ControlRef control = NULL;
		if ( noErr != GetEventParameter( event_in, kEventParamControlRef,
				typeControlRef, NULL, sizeof control, NULL, &control ) )
			control = NULL;
		
		if ( control == prev_button || control == pause_button || control == next_button ||
				control == volume_slider )
		{
			ClickActivationResult result = kDoNotActivateAndHandleClick;
			
			// to do: remove? (pause button already brings app to front when choosing music)
			//if ( control == pause_button && !music_album )
			//  result = kActivateAndHandleClick;
			debug_if_error( SetEventParameter( event_in, kEventParamClickActivation,
					typeClickActivationResult, sizeof result, &result ) );
			return true;
		}
	}
	
	return false;
}

void Player_Window::show_scope()
{
	if ( !scope ) {
		scope.reset( new Player_Scope );
		scope->player = &player;
	}
	if ( !scope->visible() ) {
		// don't deactivate keyboard focus of player window
		SendBehind( scope->window(), this->window() );
		scope->show();
	}
}

bool Player_Window::handle_command( const HICommandExtended& cmd )
{
	switch ( cmd.commandID )
	{
		case 'ECur':
			if ( music_album ) {
				player.extend_track( 60 * 1 );
				update_time();
			}
			return true;
		
		case 'Chan':
			channels_window.show( true );
			return true;
		
		case 'Scop':
			show_scope();
			break;
		
		case 'Expo':
			if ( music_album ) {
				scoped_restorer<bool> r( &recording );
				recording = true;
				uncache();
				record_track( current(), mute_mask );
			}
			return true;
		
		case 'DPau':
		case 'Paus':
			if ( music_album )
				toggle_pause();
			else
				choose_music();
			return true;
		
		case 'DPrv':
		case 'Prev':
			prev_track();
			return true;
		
		case 'DNxt':
		case 'Next':
			if ( !enable_fast_forward( false ) )
				next_track();
			return true;
		
		case kHICommandOpen:
			choose_music();
			return true;
		
		case 'Clos':
			player.pause( false ); // graphics can take a while, so pause first
			show( false );
			stop( true );
			return true;
		
		case 'Zoom':
			show_info( !prefs.show_info );
			return true;
			
		case 'SkpS':
			set_skip_tracks( !prefs.songs_only );
			return true;
		
		case 'Shuf':
			set_shuffle( !prefs.shuffle );
			setup_changed();
			return true;
		
		case 'Len0':
		case 'Len1':
		case 'Len2':
		case 'Len3':
			set_length( cmd.commandID - 'Len0' );
			setup_changed();
			return true;
	}
	
	return Carbon_Window::handle_command( cmd );
}

bool Player_Window::handle_event( EventRef event )
{
	UInt32 kind = GetEventKind( event );
	switch ( GetEventClass( event ) )
	{
		case kEventClassKeyboard:
			if ( IsUserCancelEventRef( event ) )
			{
				stop( true );
				return true;
			}
			
			switch ( kind )
			{
				case kEventRawKeyRepeat:
				case kEventRawKeyDown:
				{
					UInt32 code = 0;
					debug_if_error( GetEventParameter( event, kEventParamKeyCode,
							typeUInt32, NULL, sizeof code, NULL, &code ) );
					bool repeat = (kind == kEventRawKeyRepeat);
					switch ( code )
					{
						case 123: // left
						case 124: { // right
							bool prev = (code == 123);
							Button_Pusher pusher( prev ? prev_button : next_button,
									repeat ? 1 : 4 );
							
							if ( !music_album )
								repeat = false;
							
							bool old_auto = auto_unpause;
							auto_unpause |= repeat;
							if ( !(prev ? prev_track() : next_track()) ) {
								auto_unpause = false;
							}
							else if ( auto_unpause ) {
								track_timer.set_next_time( 0.2 );
								playing = true;
								playing_changed();
							}
							
							return true;
						}
					}
					
					char key = 0;
					debug_if_error( GetEventParameter( event, kEventParamKeyMacCharCodes,
							typeChar, NULL, sizeof key, NULL, &key ) );
					switch ( key )
					{
						case ' ': // space
							if ( !repeat )
							{
								if ( music_album ) {
									(Button_Pusher( pause_button, 4 )),
											toggle_pause();
								}
								else {
									press_button( pause_button );
									choose_music();
								}
							}
							return true;
						
						case '=':
						case '+':
							adjust_volume( 1 );
							return true;
						
						case '-':
							adjust_volume( -1 );
							return true;
						
					}
					break;
				}
			}
			break;
	}
	
	return Carbon_Window::handle_event( event );
}


// Playlist handling

void Player_Window::open_error()
{
	player.pause( false );
	playing = false;
	
	if ( fast_forwarding )
		enable_fast_forward( false );
	
	history.erase( history.begin() + history_pos );
	bool double_error = true;
	
	// try next file
	if ( has_future() ) {
		try {
			if ( play_current_() )
				double_error = false;
		}
		catch ( ... ) {
		}
	}
	
	// try previous file
	if ( double_error && history_pos > 0 ) {
		history_pos--;
		try {
			auto_unpause = true; // don't start track
			if ( play_current_() )
				double_error = false;
		}
		catch ( ... ) {
		}
		auto_unpause = false;
	}
	
	if ( double_error ) {
		history_pos = -1;
		stop();
	}
	playing_changed();
}

bool Player_Window::play_current()
{
	bool result = true;
	
	try {
		result = play_current_();
	}
	catch ( Error_Message& e )
	{
		FSRef file = current();
		open_error();
		throw_file_error( e.str, file );
	}
	catch ( ... )
	{
		open_error();
		throw;
	}
	
	return result;
}

void Player_Window::uncache()
{
	if ( music_album )
		music_album->uncache();
}

void Player_Window::choose_music()
{
#error here
	app_to_front();
	begin_drop();
	if ( select_music_items( &new_queue_, "Play Music",
			"Select game music files and playlist folders to play", "Play" ) )
	{
		end_drop();
	}
}

void Player_Window::items_dropped( DragReference drag )
{
	begin_drop();
	Drag_Handler::items_dropped( drag );
	end_drop();
}

bool Player_Window::handle_drag( const FSRef& spec, OSType type, bool take_action )
{
	if ( !is_dir_type( type ) && !identify_music_file( spec, type ) )
		return false;
	
	if ( take_action )
		handle_drop( spec );
	
	return true;
}

void Player_Window::begin_drop()
{
	reset_container( new_queue_ );
	uncache();
}

void Player_Window::handle_drop( const FSRef& path )
{
	append_playlist( path, new_queue_ );
}

bool Player_Window::end_drop( bool immediate )
{
	if ( new_queue_.empty() )
		return false;
	
	if ( !music_album || immediate )
	{
		queue.swap( new_queue_ );
		reset_container( new_queue_ );
		
		if ( history_pos + 1 == history.size() + 1 ) // avoid comparison of negative
			history_pos--; // was past end
		
		history.resize( history_pos + 1 );
		
		scoped_restorer<bool> r( &prefs.songs_only );
		if ( queue.size() == 1 )
			prefs.songs_only = false; // disable songs_only in case single non-song track was played
		next_track();
		if ( !playing )
			return false;
	}
	else
	{
		queue.insert( queue.end(), new_queue_.begin(), new_queue_.end() );
		reset_container( new_queue_ );
	}
	
	return true;
}

track_ref_t& Player_Window::current() {
	assert( history_pos < history.size() );
	return history [history_pos];
}

bool Player_Window::has_future()
{
	while ( history_pos < history.size() )
	{
		if ( FSResolveAliasFileExists( history [history_pos] ) )
			return true;
		history.erase( history.begin() + history_pos );
	}
	
	while ( !queue.empty() )
	{
		int index = (prefs.shuffle ? random( queue.size() ) : 0);
		track_ref_t track = queue [index];
		queue.erase( queue.begin() + index );
		if ( FSResolveAliasFileExists( track ) )
		{
			history.push_back( track );
			if ( history.size() > history_max ) {
				int n = history.size() - history_max;
				history_pos -= n;
				history.erase( history.begin(), history.begin() + n );
			}
			return true;
		}
	}
	
	return false;
}

bool Player_Window::next_track()
{
	history_pos++;
	
	while ( has_future() )
	{
		if ( play_current() )
			return true;
		
		// remove skipped track from history
		assert( history_pos < history.size() );
		history.erase( history.begin() + history_pos );
	}
	
	// no more tracks
	history_pos = history.size();
	stop();
	return false;
}

bool Player_Window::prev_track()
{
	if ( music_album && (player.elapsed() > 3 || !playing) ) {
		if ( start_track() )
			return true;
	}
	
	while ( history_pos > 0 )
	{
		history_pos--;
		if ( !FSResolveAliasFileExists( current() ) ) {
			history.erase( history.begin() + history_pos );
		}
		else if ( play_current() )
			return true;
	}
	
	// no more tracks
	if ( history_pos != -1 ) {
		history_pos = -1;
		stop();
	}
	return false;
}

bool Player_Window::play_current_()
{
	if ( scope && scope->visible() )
		scope->clear( true );
	
	track_ref_t& track_ref = current();
	FSRef path = FSResolveAliasFileChk( track_ref );
	
	OSType file_type = 0;
	
	Cat_Info info;
	
	// see if new track is in a different file than current one
	if ( !music_album || (0 != std::memcmp( &path, &album_path, sizeof album_path ) &&
			0 != FSCompareFSRefs( &path, &album_path )) )
	{
		// current album might be bloated RAR archive
		uncache();
		
		HFSUniStr255 name;
		info.read( track_ref, kFSCatInfoFinderInfo, &name );
		file_type = identify_music_file( path, info.finfo().fileType );
		if ( !file_type )
			return false;
		
		// won't load album if non-music file or archive with no music
		unique_ptr<Music_Album> album( load_music_album( path, file_type, name ) );
		if ( !album )
			return false;df
		
		if ( file_type == gzip_type )
			file_type = album->music_type();
		
		player.pause( false ); // current album might hold data used by player
		m_pMusicAlbum.reset(); // be sure current album is clear if error occurs
		
		player.play( album.get() );
		m_pMusicAlbum.reset( album.release() );
		album_path = path;
	}
	
	if ( !start_track() )
		return false; // track needs to be skipped
	
	track_changed();
	
	// change file icon
	if ( prefs.change_icon && file_type )
	{
		if ( info.finfo().fileType != file_type || info.finfo().fileCreator != gmb_creator )
		{
			try {
				info.finfo().fileType = file_type;
				info.finfo().fileCreator = gmb_creator;
				info.write( path, kFSCatInfoFinderInfo );
			}
			catch ( ... ) {
				// ignore errors (might be write-protected)
				check( false );
			}
		}
	}
	
	return true;
}

bool Player_Window::start_track()
{
	bool was_playing = playing;
	playing = false; // in case error occurs
	player.start_track( current().track );
	playing = was_playing;
	if ( prefs.songs_only && !music_album->info().is_song )
		return false;
	
	if ( !auto_unpause ) {
		player.resume();
		playing = true;
		playing_changed();
	}
	update_time();
	return true;
}

void Player_Window::stop( bool clear_history )
{
	if ( clear_history ) {
		reset_container( history );
		history_pos = -1;
		reset_container( queue );
	}
	
	assert( history_pos == -1 || history_pos == history.size() );
	if ( history_pos + 1 > history.size() + 1 ) // avoid comparing negatives
		history_pos = history.size();
	
	player.stop();
	
	stopped();
	
	// to do: return immediately if already stopped (otherwise ui flashes)
	
	music_album.reset();
}

#endif // COMPILE_GMB_GUI


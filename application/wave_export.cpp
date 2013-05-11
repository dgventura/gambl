
// Game_Music_Box 0.5.2. http://www.slack.net/~ant/game-music-box

#include "wave_export.h"

//#include <Navigation.h>
#include "Progress_Window.h"
#include "Errors_Window.h"
#include "Wave_Writer.h"
#include "music_util.h"
#include "file_util.h"
#include "File_Emu.h"
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

const int stereo = 2;

// util

class CFStringHolder {
	CFStringRef ref;
public:
	CFStringHolder( CFStringRef r ) : ref( r ) { }
	~CFStringHolder() { CFRelease( ref ); }
};

static void cfstr_to_unistr( CFStringRef in, HFSUniStr255* out ) {
	out->length = CFStringGetLength( in );
	CFRange range = { 0, out->length };
	CFStringGetCharacters( in, range, out->unicode );
}

static bool ask_save_file( FSRef* dir, HFSUniStr255* name, CFStringRef initial_name = NULL )
{
	// run dialog
	Nav_Options options( PROGRAM_NAME );
	options.actionButtonLabel = CFSTR( "Record" );
	options.windowTitle = CFSTR( "Record Track" );
	//options.message = CFSTR( "Record to sound file" );
	options.saveFileName = initial_name;
	options.optionFlags &= ~kNavAllowStationery;
	NavDialogRef dialog;
	throw_if_error( NavCreatePutFileDialog( &options, 'WAVE', 'TVOD', default_nav_handler(),
			NULL, &dialog ) );
	NavDialog_Holder holder( dialog );
	throw_if_error( NavDialogRun( dialog ) );
	
	// get reply
	Nav_Reply reply( dialog );
	if ( !reply.validRecord )
		return false;
	require( reply.size() == 1 );
	*dir = reply [0];
	cfstr_to_unistr( reply.saveFileName, name );
	if ( reply.replacing ) {
		FSRef file;
		FSMakeFSRefExists( *dir, *name, &file );
		throw_file_error( FSDeleteObject( &file ), file );
	}
	return true;
}

bool choose_folder( FSRef* dir )
{
	// run dialog
	Nav_Options options( PROGRAM_NAME );
	options.actionButtonLabel = CFSTR( "Record" );
	options.windowTitle = CFSTR( "Record Playlist" );
	options.message = CFSTR( "Select destination folder for recordings" );
	NavDialogRef dialog;
	throw_if_error( NavCreateChooseFolderDialog( &options, default_nav_handler(),
			NULL, NULL, &dialog ) );
	NavDialog_Holder holder( dialog );
	throw_if_error( NavDialogRun( dialog ) );
	
	// get reply
	Nav_Reply reply( dialog );
	if ( !reply.validRecord )
		return false;
	require( reply.size() == 1 );
	*dir = reply [0];
	
	return true;
}

static void write_wave( File_Emu& emu, const FSRef& dir, const HFSUniStr255& name,
		long min_length = -1, Progress_Hook* hook = NULL )
{
	FSRef out_path = create_file( dir, name, 'WAVE', 'TVOD' );
	File_Deleter deleter( out_path );
	
	Mac_File file( out_path );
	Wave_Writer wave( &file );
	
	runtime_array<blip_sample_t> buf( 8 * 1024L );
	
	// Pad beginning with silence
	std::memset( buf.begin(), 0, buf.size() * sizeof buf [0] );
	wave.write( buf.begin(), buf.size() / 2 );
	
	int idle_delay = 1;
	long silence_trim = 0;
	double progress_factor = 1.0 / wave_sample_rate / stereo / emu.track_length();
	bool skip_silence = true;
	while ( !emu.is_done() )
	{
		if ( emu.play( buf.begin(), buf.size(), skip_silence ) )
		{
			// If all buffers after this are silent, file will be trimmed to this point
			silence_trim = wave.sample_count() + stereo * wave_sample_rate / 2;
			
			// Stop skipping initial silence
			skip_silence = false;
		}
		
		if ( !skip_silence )
			wave.write( buf.begin(), buf.size() );
		
		// Periodically call idle function
		if ( !--idle_delay )
		{
			idle_delay = 10;
			if ( hook )
			{
				double prog = emu.sample_count() * progress_factor;
				if ( prog > 1.0 )
					prog = 1.0;
				hook->update( prog );
				if ( hook->give_time() )
					return;
			}
		}
	}
	
	if ( silence_trim && silence_trim < wave.sample_count() )
		wave.trim( silence_trim );
	
	wave.finish( wave_sample_rate );
	
	// Keep wave only if at least half a second was written
	min_length *= 2;
	if ( min_length < 0 )
		min_length = 1;
	if ( wave.sample_count() >= min_length * stereo * wave_sample_rate )
		deleter.clear();
}

// If current exception is disk full, re-throw more descriptive message, otherwise
// do nothing.
void handle_disk_full()
{
	try {
		throw;
	}
	catch ( Error_Code& e ) {
		if ( e.code == dskFulErr )
			throw_error( "Recording was stopped because the disk is full. "
					"Free some disk space or choose a different destination folder." );
	}
}

void record_track( const track_ref_t& track, int mute_mask )
{
	// to do: make copy of track if use persists after nav dialog
	
	unique_ptr<Music_Album> album(
			load_music_album( FSResolveAliasFileChk( track ) ) );
	if ( !album ) {
		check( false );
		return;
	}
	
	// open file in emulator and start track
	File_Emu emu;
	emu.change_setup( prefs );
	emu.load( album.get(), wave_sample_rate );
	emu.set_mute( mute_mask );
	emu.start_track( track.track );
	album->uncache();
	
	// generate initial filename
	char name [256];
	std::strcpy( name, album->info().filename );
	if ( album->track_count() > 1 )
	{
		name [31 - 3] = 0;
		char num [32];
		num [0] = '-';
		num_to_str( track.track + 1, num + 1, 2 );
		std::strcat( name, num );
	}
	
	// get save location
	CFStringRef str = CFStringCreateWithCString( NULL, name, kCFStringEncodingASCII );
	CFStringHolder holder( str );
	FSRef dir;
	HFSUniStr255 filename;
    
#ifdef GMB_COMPILE_GUI
	if ( ask_save_file( &dir, &filename, str ) )
	{
		// write wave
		Progress_Window pw( "Record Track" );
		char new_name [256];
		filename_to_str( filename, new_name );
		pw.set_text( new_name );
		pw.set_total( 1 );
		try {
			write_wave( emu, dir, filename, -1, &pw );
		}
		catch ( ... ) {
			handle_disk_full();
			throw;
		}
	}
#endif
}

void record_track_( const track_ref_t& track, const FSRef& out_dir,
		const File_Emu::setup_t& setup, Progress_Hook& pw )
{
	// open file and emu
	unique_ptr<Music_Album> album(
			load_music_album( FSResolveAliasFileChk( track ) ) );
	if ( !album )
		return;
	
	File_Emu emu;
	emu.change_setup( setup );
	emu.load( album.get(), wave_sample_rate );
	
	// start track
	int track_index = track.track;
	if ( track_index >= album->track_count() )
		track_index = 0;
	emu.start_track( track_index );
	album->uncache();
	
	if ( prefs.songs_only && !album->info().is_song )
		return;
	
	HFSUniStr255 name;
	
	// create output directory
	str_to_filename( album->info().game, name );
	if ( name.length > max_filename )
		name.length = max_filename;
	
	sanitize_filename( name );
	FSRef dir = out_dir;
	if ( !FSMakeFSRefExists( out_dir, name, &dir ) )
		FSCreateDirectoryUnicodeChk( out_dir, name, 0, NULL, &dir );
	
	// name output file
	char str [256];
	str [0] = 0;
	if ( album->track_count() > 1 )
		num_to_str( track_index + 1, str_end( str ), -2 );
	const char* track_name = album->info().song;
	if ( !*track_name && album->track_count() == 1 )
		track_name = album->info().filename;
	if ( *track_name ) {
		std::strcat( str, " " );
		std::strcat( str, track_name );
	}
	
	str [max_filename] = 0;
	str_to_filename( str, name );
	sanitize_filename( name );
	
	if ( !FSMakeFSRefExists( dir, name ) )
	{
		// write wave
		char text [256];
		std::strcpy( text, album->info().game );
		std::strcat( text, ": " );
		std::strcat( text, str );
		pw.set_text( text );
		write_wave( emu, dir, name, prefs.songs_only ? 10 : -1, &pw );
	}
}

bool select_music_items( Music_Queue* queue, const char* title, const char* message,
		const char* button )
{
	// to do: find way to allow selection of multiple folders

#ifdef GMB_COMPILE_GUI
	// run dialog
	Nav_Options options( PROGRAM_NAME );
	options.windowTitle = __CFStringMakeConstantString( title );
	options.message = __CFStringMakeConstantString( message );
	options.actionButtonLabel = __CFStringMakeConstantString( button );
	options.optionFlags |= kNavAllowMultipleFiles;
	NavDialogRef dialog;
	throw_if_error( NavCreateChooseObjectDialog( &options, default_nav_handler(),
			NULL, NULL, NULL, &dialog ) );
	NavDialog_Holder holder( dialog );
	throw_if_error( NavDialogRun( dialog ) );
	
	// get reply
	Nav_Reply reply( dialog );
	if ( !reply.validRecord )
		return false;
	
	for ( int i = 0; i < reply.size(); i++ )
		append_playlist( reply [i], *queue );
#endif
    
	return true;
}


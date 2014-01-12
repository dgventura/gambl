
// Game_Music_Box 0.5.2. http://www.slack.net/~ant/game-music-box

#include "wave_export.h"

#include "Wave_Writer.h"
#include "music_util.h"
#include "FileUtilities.h"
#include "File_Emu.h"
#include "prefs.h"
#import "NSString+wstring_additions.h"

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

bool ask_save_file( std::wstring& dir, std::wstring& name, const char* const initial_name )
{
    NSSavePanel* saveDialog = [NSSavePanel savePanel];
    [saveDialog setCanCreateDirectories:YES];
    [saveDialog setAllowsOtherFileTypes:NO];
    [saveDialog setAllowedFileTypes:[[NSArray alloc] initWithObjects:@"wav", nil]];
    [saveDialog setNameFieldStringValue:[NSString stringWithCString:initial_name encoding:NSASCIIStringEncoding]];
    [saveDialog setTitle:@"Save WAV file as"];
    
    NSInteger clicked = [saveDialog runModal];
    
    if ( clicked == NSFileHandlingPanelOKButton )
    {
        Boolean bIsDirectory = FALSE;
        NSString *fullPath = [[saveDialog URL] path];
        std::wstring strPath = [[fullPath stringByDeletingLastPathComponent] getwstring];
        //RAD dir.OpenFileFromPath( strPath, "w" );
        str_to_filename( [[fullPath lastPathComponent] UTF8String], name );
        //RADassert( &dir );
        return true;
    }

    return false;
}

bool choose_folder( GaMBLFileHandle* dir )
{
#if 0
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
#endif
	return true;
}

void write_wave( File_Emu& emu, const std::wstring& dir, const std::wstring& name,
		long min_length, Progress_Hook* hook, bool bKeepStartingSilence )
{
	GaMBLFileHandle out_path = create_file( dir, name );//RAD, 'WAVE', 'TVOD' );
//RAD	File_Deleter deleter( out_path );
	
    std::wstring strPath;
    out_path.GetFilePath( strPath, true );
	Mac_File file( strPath );
	Wave_Writer wave( &file );
	
	runtime_array<blip_sample_t> buf( 8 * 1024L );
	
	// Pad beginning with silence
	std::memset( buf.begin(), 0, buf.size() * sizeof buf [0] );
	wave.write( buf.begin(), buf.size() / 2 );
	
	int idle_delay = 1;
	long silence_trim = 0;
	double progress_factor = 1.0 / wave_sample_rate / stereo / emu.track_length();
	bool skip_silence = !bKeepStartingSilence;
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
				//TODO: THREADING hook->update( prog );
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
//RAD	if ( wave.sample_count() >= min_length * stereo * wave_sample_rate )
//RAD		deleter.clear();
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


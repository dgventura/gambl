
// Record track(s) to wave sound file(s)

// Game_Music_Box 0.5.2. Copyright (C) 2005 Shay Green. GNU LGPL license.

#ifndef WAVE_EXPORT_H
#define WAVE_EXPORT_H

#include "common.h"
#include "music_util.h"
#include "File_Emu.h"

class Music_Album;
class File_Emu;
class Progress_Hook;

const long wave_sample_rate = 44100;

void handle_disk_full();

bool choose_folder( FSRef* );

bool select_music_items( Music_Queue*, const char* title, const char* message,
		const char* button );

void record_track_( const track_ref_t& track, const FSRef& out_dir,
		const File_Emu::setup_t&, Progress_Hook& );

// Ask for output folder then record tracks
void record_tracks( const track_ref_t*, int count );

// Ask for output file then record track
void record_track( const track_ref_t&, int mute_mask );

#endif


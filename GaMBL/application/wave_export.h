
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

bool ask_save_file( FSRef* dir, HFSUniStr255* name, const char* const initial_name = NULL );
void write_wave( File_Emu& emu, const FSRef& dir, const HFSUniStr255& name,
                long min_length = -1, Progress_Hook* hook = NULL, bool bKeepStartingSilence = true );
bool select_music_items( Music_Queue*, const char* title, const char* message,
		const char* button );

// Ask for output folder then record tracks
void record_tracks( const track_ref_t*, int count );

#endif



// Sound debugging utility that writes samples to a debugging sound file, even at
// interrupt time

// Game_Music_Box 0.5.2. Copyright (C) 2005 Shay Green. GNU LGPL license.

#ifndef SOUND_DEBUGGER_H
#define SOUND_DEBUGGER_H

#include "common.h"

// Create "debug.wav" to record samples to.
void open_sound_debugger();

// Write samples to "debug.wav". Samples can be written at interrupt time.
void write_sound_debugger( const BOOST::int16_t*, long count );

#endif


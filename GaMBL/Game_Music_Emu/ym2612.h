
// Sega Genesis YM2612 FM Sound Chip Emulator

// Game_Music_Emu 0.2.6. Copyright (C) 2004-2005 Shay Green. GNU LGPL license.
// Copyright (C) 2002 Stéphane Dallongeville

#ifndef YM2612_H
#define YM2612_H

#include "blargg_common.h"

struct YM2612_Impl;

class YM2612_Emu {
	YM2612_Impl* impl;
public:
	YM2612_Emu() { impl = NULL; }
	~YM2612_Emu();
	
	blargg_err_t set_rate( long sample_rate, double clock_factor );
	
	void reset();
	
	enum { channel_count = 6 };
	void mute_voices( int mask );
	
	void write0( int addr, int data );
	void write1( int addr, int data );
	
	void run_timer( int );
	
	typedef BOOST::int16_t sample_t;
	void run( sample_t*, int count );
};

#endif



// Sega Genesis GYM music file emulator

// Game_Music_Emu 0.2.6. Copyright (C) 2004-2005 Shay Green. GNU LGPL license.

#ifndef GYM_EMU_H
#define GYM_EMU_H

#include "Fir_Resampler.h"
#include "Blip_Buffer.h"
#include "Music_Emu.h"
#include "Sms_Apu.h"
#include "ym2612.h"

class Gym_Emu : public Music_Emu {
public:
	// Initialize emulator with given sample rate, gain, and oversample. A gain of 1.0
	// results in almost no clamping. Default gain roughly matches volume of other emulators.
	// The FM chip is synthesized at an increased rate governed by the oversample factor,
	// where 1.0 results in no oversampling and > 1.0 results in oversampling.
	blargg_err_t init( long sample_rate, double gain = 1.5, double oversample = 5 / 3.0 );
	
	struct header_t {
	    char tag [4];
	    char song [32];
	    char game [32];
	    char copyright [32];
	    char emulator [32];
	    char dumper [32];
	    char comment [256];
	    byte loop [4];
	    byte packed [4];
	    
	    enum { track_count = 1 }; // one track per file
		enum { author = 0 }; // no author field
	};
	BOOST_STATIC_ASSERT( sizeof (header_t) == 428 );
	
	// Load GYM
	blargg_err_t load( Emu_Reader& );
	
	// Load GYM using already-loaded header and remaining data
	blargg_err_t load( header_t const&, Emu_Reader& );
	
	// Load GYM using pointer to file data. Keeps pointer to data.
	blargg_err_t load( void const* data, long size );
	
	// Header for currently loaded GYM
	header_t const& header() const { return header_; }
	
	// Length of track in seconds (0 if looped). If loop_start is not NULL,
	// sets *loop_start to the beginning of the loop (-1 if not looped).
	int track_length( int* loop_start = NULL ) const;
	
public:
	Gym_Emu();
	~Gym_Emu();
	void mute_voices( int );
	void start_track( int );
	void play( long count, sample_t* );
	const char** voice_names() const;
	void skip( long count );
private:
	// sequence data begin, loop begin, current position, end
	const byte* data;
	const byte* loop_begin;
	const byte* pos;
	const byte* data_end;
	long loop_offset;
	long loop_remain; // frames remaining until loop beginning has been located
	byte* mem;
	blargg_err_t load_( const void* file, long data_offset, long file_size );
	
	// frames
	double oversample;
	int pairs_per_frame;
	int oversamples_per_frame;
	void parse_frame();
	void play_frame( sample_t* );
	void mix_samples( sample_t* );
	
	// dac (pcm)
	int last_dac;
	int prev_dac_count;
	bool dac_enabled;
	bool dac_disabled;
	void run_dac( int );
	
	// sound
	int extra_pos; // extra samples remaining from last read
	Blip_Buffer blip_buf;
	YM2612_Emu fm;
	header_t header_;
	Blip_Synth<blip_med_quality,1> dac_synth;
	Sms_Apu apu;
	Fir_Resampler resampler;
	byte dac_buf [1024];
	enum { sample_buf_size = 4096 };
	sample_t sample_buf [sample_buf_size];
	
	void unload();
};

#endif


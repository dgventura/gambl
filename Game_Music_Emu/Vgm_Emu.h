
// Multi-format VGM music emulator with support for SMS PSG and Mega Drive FM

// Game_Music_Emu 0.2.6. Copyright (C) 2003-2005 Shay Green. GNU LGPL license.

#ifndef VGM_EMU_H
#define VGM_EMU_H

#include "Vgm_Emu_Impl.h"
#include "ym2612.h"

class Ym_2612;

class Ym_2612 : public Ym_Emu, public YM2612_Emu {
	typedef YM2612_Emu emu;
public:
	virtual void write( int addr, int data ) { emu::write0( addr, data ); }
	virtual void write1( int addr, int data ) { emu::write1( addr, data ); }
	virtual void run( int count );
	short* out;
};

class Vgm_Emu : public Vgm_Emu_Impl {
public:
	
	Vgm_Emu();
	~Vgm_Emu();
	
	void start_track( int );
	const char** voice_names() const;
	void play( long count, sample_t* );
	void mute_voices( int mask );
protected:
	// overrides
	void update_eq( blip_eq_t const& );
	void write_pcm( blip_time_t, int );
	blargg_err_t setup_fm();
private:
	Ym_2612 ym2612_;
	
	long buf_size;
	long buf_pos;
	long sample_buf_size;
	typedef sample_t sample_pair_t [2];
	sample_pair_t* sample_buf;
	int read_samples( int count, sample_t* out );
	void mix_samples( sample_t* );
	void refill_buf( sample_t* );
	
	Blip_Synth<blip_med_quality,1> dac_synth;
};

#endif


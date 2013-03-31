
// VGM implementation. Could be used to play PSG-only VGMs without needing FM source code.

// Game_Music_Emu 0.2.6. Copyright (C) 2003-2005 Shay Green. GNU LGPL license.

#ifndef VGM_EMU_IMPL_H
#define VGM_EMU_IMPL_H

#include "abstract_file.h"
#include "Multi_Buffer.h"
#include "Classic_Emu.h"
#include "Sms_Apu.h"
#include "ym2612.h"

class Ym_Emu {
public:
	virtual ~Ym_Emu() { }
	
	virtual void write( int addr, int data ) { }
	virtual void write1( int addr, int data ) { }
	virtual void run( int count ) { }
};

struct ym_holder
{
	int last_time;
	int last_fm_time;
	Ym_Emu* emu;
	
	ym_holder() { emu = NULL; }
	void disable() { last_time = INT_MAX; }
	void begin_frame() { last_time = 0; last_fm_time = 0; }
	int fm_time( int time ) const { return time + (time >> 1); }
	void run_until( int time );
};

class Vgm_Emu_Impl : public Classic_Emu {
public:
	typedef BOOST::uint8_t byte;
	
	Vgm_Emu_Impl();
	~Vgm_Emu_Impl();
	
	struct header_t
	{
		char signature [4];
		byte data_size [4];
		byte version [4];
		byte psg_rate [4];
		byte ym2413_rate [4];
		byte gd3_offset [4];
		byte track_duration [4];
		byte loop_offset [4];
		byte loop_duration [4];
		byte frame_rate [4];
		byte noise_feedback [2];
		byte noise_width;
		byte unused1;
		byte ym2612_rate [4];
		byte ym2151_rate [4];
		byte data_offset [4];
		byte unused2 [8];
		
	    enum { track_count = 1 }; // one track per file
		
		// get track information from gd3 data
		enum { game = 0 };
		enum { song = 0 };
		enum { author = 0 };
		enum { copyright = 0 };
	};
	BOOST_STATIC_ASSERT( sizeof (header_t) == 64 );
	
	// True if VGM uses FM sound
	static bool uses_fm_sound( header_t const& );
	
	virtual blargg_err_t init( long sample_rate );
	virtual blargg_err_t init( Multi_Buffer* );
	
	// Load VGM
	blargg_err_t load( Emu_Reader& );
	
	// Load VGM using already-loaded header and remaining data
	blargg_err_t load( header_t const&, Emu_Reader& );
	
	// Load VGM using already-loaded header and pointer to remaining data.
	// Keeps pointer to data.
	blargg_err_t load( const header_t&, void const* data, long size );
	
	// Header for currently loaded VGM
	header_t const& header() const { return header_; }
	
	// Pointer to gd3 data, or NULL if none. Optionally returns size of data.
	// Checks for GD3 header and that version is less than 2.0.
	byte const* gd3_data( int* size_out = NULL ) const;
	
public:
	// Music_Emu
	void start_track( int );
	const char** voice_names() const;
protected:
	// Classic_Emu
	void set_voice( int, Blip_Buffer*, Blip_Buffer*, Blip_Buffer* );
	void update_eq( blip_eq_t const& );
protected:
	friend class Vgm_Emu; private:
	enum { vgm_rate = 44100 };
	ym_holder ym2612;
	Stereo_Buffer* blip_buf;
	header_t header_;
	blip_time_t run( int, bool* );
	virtual blargg_err_t setup_fm();
	virtual void write_pcm( blip_time_t, int ) { }
private:
	typedef int fm_time_t;
	typedef int vgm_time_t;
	
	// vgm data
	byte const* data;
	byte const* loop_begin;
	byte const* data_end;
	byte* owned_data;
	void unload();
	blargg_err_t load_( const header_t&, void const* data, long size );
	
	long psg_factor;
	long to_psg_time( vgm_time_t ) const;
	
	vgm_time_t vgm_time;
	byte const* pos;
	void run_commands( vgm_time_t );
	
	byte const* pcm_data;
	byte const* pcm_pos;
	int dac_amp;
	int dac_enabled;
	
	Ym_Emu null_ym;
	Sms_Apu psg;
};

inline void ym_holder::run_until( int time )
{
	if ( time > last_time )
	{
		last_time = time;
		time = fm_time( time );
		int count = time - last_fm_time;
		last_fm_time = time;
		emu->run( count );
	}
}
	
inline blargg_err_t Vgm_Emu_Impl::load( const header_t& h, void const* data, long size )
{
	unload();
	return load_( h, data, size );
}

#endif


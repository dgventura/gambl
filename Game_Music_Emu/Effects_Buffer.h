
// Multi-channel effects buffer with panning, echo and reverb effects

// Game_Music_Emu 0.2.6. Copyright (C) 2003-2005 Shay Green. GNU LGPL license.

#ifndef EFFECTS_BUFFER_H
#define EFFECTS_BUFFER_H

#include "Multi_Buffer.h"

// Effects_Buffer uses several buffers and outputs stereo sample pairs.
class Effects_Buffer : public Multi_Buffer {
public:
	// If center_only is true, only center buffers are created and
	// less memory is used.
	Effects_Buffer( bool center_only = false );
	~Effects_Buffer();
	
	// Channel  Effect    Center Pan
	// ---------------------------------
	//    0     reverb       pan_1
	//    1     reverb       pan_2
	//    2      echo         -
	//    3      echo         -
	//    4      echo         -
	
	// Channel configuration
	struct config_t {
		double pan_1;           // -1.0 = left, 0.0 = center, 1.0 = right
		double pan_2;
		double echo_delay;      // msec
		double echo_level;      // 0.0 to 1.0
		double reverb_delay;    // msec
		double delay_variance;  // difference between left/right delays (msec)
		double reverb_level;    // 0.0 to 1.0
		bool effects_enabled;   // if false, use optimized simple mixer
		config_t();
	};
	
	// Set configuration of buffer
	void config( const config_t& );
	
	// See Multi_Buffer.h
	blargg_err_t set_sample_rate( long samples_per_sec, int msec = blip_default_length );
	void clock_rate( long );
	void bass_freq( int );
	void clear();
	channel_t channel( int );
	void end_frame( blip_time_t, bool was_stereo = true );
	long read_samples( blip_sample_t*, long );
	long samples_avail() const;
	
// End of public interface
private:
	typedef long fixed_t;
	
	enum { max_buf_count = 7 };
	Blip_Buffer bufs [max_buf_count];
	enum { chan_count = 5 };
	channel_t channels [chan_count];
	config_t config_;
	long stereo_remain;
	long effect_remain;
	int buf_count;
	bool effects_enabled;
	
	blip_sample_t* reverb_buf;
	blip_sample_t* echo_buf;
	int reverb_pos;
	int echo_pos;
	
	struct {
		fixed_t pan_1_levels [2];
		fixed_t pan_2_levels [2];
		int echo_delay_l;
		int echo_delay_r;
		fixed_t echo_level;
		int reverb_delay_l;
		int reverb_delay_r;
		fixed_t reverb_level;
	} chans;
	
	void mix_mono( blip_sample_t*, long );
	void mix_stereo( blip_sample_t*, long );
	void mix_enhanced( blip_sample_t*, long );
	void mix_mono_enhanced( blip_sample_t*, long );
};

	inline Effects_Buffer::channel_t Effects_Buffer::channel( int i ) {
		return channels [i % chan_count];
	}
	
#endif


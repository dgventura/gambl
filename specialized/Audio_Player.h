
// Simple interface to Sound Manager. Allows one buffer at a time to be played,
// with a callback when the next one is needed.

// Game_Music_Box 0.5.2. Copyright (C) 2005 Shay Green. GNU LGPL license.

#ifndef AUDIO_PLAYER_H
#define AUDIO_PLAYER_H

#include "common.h"
#include "thread_util.h"
//#include <Sound.h>

class Audio_Player : noncopyable {
public:
	Audio_Player();
	~Audio_Player();
	
	// Set output gain, where 1.0 has no effect
	void set_gain( double );
	
	// Get sound output hardware's current sample rate in Hz (i.e. 44100)
	double hw_sample_rate() const;
	
	// Change setup
	typedef void (*callback_t)( void* );
	void setup( long sample_rate, bool stereo, callback_t func, void* data = NULL );
	
	// Play samples then invoke callback
	typedef short sample_t;
	void play_buffer( const sample_t*, int count );
	
	// Stop playing current buffer and don't invoke callback
	void stop();
	
private:
	Virtual_Memory_Holder vm_holder;
#if GMB_COMPILE_AUDIO
	ExtSoundHeader snd_header;
	SndChannelPtr chan;
	callback_t callback;
	void* callback_data;
	
	static pascal void chan_callback( SndChannelPtr, SndCommand* );
#endif // #if GMB_COMPILE_AUDIO
};

#endif


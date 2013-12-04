
// Plays game music files through sound output

// Game_Music_Box 0.5.2. Copyright (C) 2005 Shay Green. GNU LGPL license.

#ifndef EMUINTERFACE_H
#define EMUINTERFACE_H

#include "common.h"

#include "File_Emu.h"
#include "AudioInterface.h"

class Music_Album;

class EmuInterface {
public:
	EmuInterface();
	~EmuInterface();
	
	struct setup_t : File_Emu::setup_t {
		double volume;
	};
	
	void setup_changed( const setup_t& );
	
	// Keeps pointer to album until stop() is called
	void play( Music_Album* );
	void start_track( int index );
	
	// Add time to current track
	void extend_track( int seconds );
	int track_length() const;
	void stop();
	
	void pause( bool might_resume = true );
	void resume();
	bool is_done( bool bCheckOnly = false ) const;
	
	void set_mute( int );
	
	// Number of seconds current track has been played
	int elapsed() const;
	
	// Enable fast-forward. Return true if fast-forward ran since the last
	// time it was enabled.
	bool enable_fast_forward( bool );
	
	const short* scope_buffer() const;
	const Music_Emu* music_emu() const;
	
    	enum { block_size = 1024 }; //TODO: figure out why this works instead of 4096 which it used to be?
private:
	
	// synthesis, silence detection
	File_Emu emu;
	volatile int low_latency;
	volatile int mute_mask;
	volatile int silence_pending;
	volatile int was_silent;
	volatile long silence_timeout;
	long sample_rate;
	bool playing;
	void queue_block();
	
	// fast forward
	//TODO: FF Event_Loop_Timer ffwd_timer;
	volatile long ffwd_next_time;
	volatile int fast_forwarding;
	void fast_forward();
	static void fast_forward_( void* );
	
	// queue
	enum { blocks_per_sec = 44100 / block_size }; // approximate
	typedef short sample_t;
	typedef sample_t block_t [block_size];
	runtime_array<block_t> blocks;
	volatile int write_pos;
	volatile int read_pos;
	int filled_count() const;
	
	// deferred queue filler
	DeferredTask dtask;
	volatile int deferred_active;
	volatile int deferred_enabled;
	void stop_deferred();
	void deferred_callback();
	static void deferred_callback_( void* );
	
	// audio block output
	//TODO: thread Virtual_Memory_Holder this_vmholder;
	//TODO: thread Virtual_Memory_Holder blocks_vmholder;
	volatile long blocks_played;
	volatile int stop_output; // 0) done, 1) fade out, -1) stopping
	AudioInterface player;
	void play_buffer( const sample_t* p, long s );
public://TODO: hack
    void sound_callback();
	static void sound_callback_( void* );
};

inline const Music_Emu* EmuInterface::music_emu() const {
	return emu.emu();
}

inline int EmuInterface::track_length() const {
	return emu.track_length();
}

inline void EmuInterface::extend_track( int seconds ) {
	emu.extend_track( seconds );
}

#endif // EMUINTERFACE_H


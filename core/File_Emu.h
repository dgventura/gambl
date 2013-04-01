
// Loads a music file then plays a track into a buffer using current setup.
// Finds end of track or fades at a given time.

// Game_Music_Box 0.5.2. Copyright (C) 2005 Shay Green. GNU LGPL license.

#ifndef FILE_EMU_H
#define FILE_EMU_H

#include "common.h"
#include "Music_Album.h"
#include "Classic_Emu.h"
#include "Effects_Buffer.h"

class File_Emu {
public:
	File_Emu();
	~File_Emu();
	
	struct setup_t {
		double duration;
		double echo_depth;
		double treble;
		double bass;
		bool custom_sound;
        
        volatile setup_t& operator=( const setup_t& rhs ) volatile
        {
            duration = rhs.duration;
            echo_depth = rhs.echo_depth;
            treble = rhs.treble;
            bass = rhs.bass;
            custom_sound = rhs.custom_sound;
            
            return *this;
        }
	};
	void change_setup( const setup_t& );
	
	// Load new file. Keeps pointer to album.
	void load( Music_Album*, long sample_rate );
	
	// Pointer to music emulator for current file, or NULL if stopped
	const Music_Emu* emu() const;
	
	// Set channel muting mask (0 = no muting)
	void set_mute( int mask );
	
	// (Re)start track
	void start_track( int index );
	
	// Extend length of current track
	void extend_track( int seconds );
	
	// Number of seconds until current track will be faded out, if it hasn't
	// gone silent before then.
	int track_length() const;
	
	// Fill buffer with 'count' samples. True if buffer wasn't completely
	// silent or wasn't checked for silence.
	typedef Music_Emu::sample_t sample_t;
	bool play( sample_t*, long count, bool force_check_silence );
	
	// Skip 'count' samples
	void skip( long count );
	
	// Set sample count to 0
	void reset_sample_count();
	
	// Number of samples played/skipped
	long sample_count() const;
	
	// True if track is done playing
	bool is_done() const;
	
	// Stop playing and unload current album
	void stop();
	
	
// End of public interface
private:
	BOOST::scoped_ptr<Music_Emu> music_emu;
	Classic_Emu* volatile classic_emu;
	Music_Album* music_album;
	
	Effects_Buffer::config_t effects_config;
	Classic_Emu::equalizer_t standard_eq;
	Effects_Buffer effects_buffer;
	
	volatile setup_t setup_;
	volatile long sample_count_;
	volatile long last_sound;
	volatile long fade_time;
	volatile int setup_changed;
	volatile int detect_silence;
	volatile int mute_mask;
	volatile int is_done_;
	double fade_factor;
	long sample_rate;
	int track_length_;
	int track_extension;
	int track;
	
	void update_length();
	void apply_setup();
};

inline bool File_Emu::is_done() const {
	return is_done_;
}
inline long File_Emu::sample_count() const {
	return sample_count_;
}
inline const Music_Emu* File_Emu::emu() const {
	return music_emu.get();
}
inline int File_Emu::track_length() const {
	return track_length_;
}
inline void File_Emu::reset_sample_count() {
	sample_count_ = 0;
}
#endif


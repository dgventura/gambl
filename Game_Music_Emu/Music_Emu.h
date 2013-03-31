
// Game music emulator interface base class

// Game_Music_Emu 0.2.6. Copyright (C) 2003-2005 Shay Green. GNU LGPL license.

#ifndef MUSIC_EMU_H
#define MUSIC_EMU_H

#include "blargg_common.h"

#include "abstract_file.h"
typedef Data_Reader Emu_Reader; // File reader base class
typedef Std_File_Reader Emu_File_Reader; // Read from file
typedef Mem_File_Reader Emu_Mem_Reader; // Read from memory block

class Music_Emu {
public:
	Music_Emu();
	virtual ~Music_Emu();
	
	// Number of voices used by currently loaded file
	int voice_count() const;
	
	// Names of voices
	virtual const char** voice_names() const;
	
	// Number of tracks. Zero if file hasn't been loaded yet.
	int track_count() const;
	
	// Start a track, where 0 is the first track. Might un-mute any muted voices.
	virtual void start_track( int ) = 0;
	
	// Mute voice n if bit n (1 << n) of mask is set
	virtual void mute_voices( int mask );
	
	// Generate 'count' samples info 'buf'
	typedef short sample_t;
	virtual void play( long count, sample_t* buf ) = 0;
	
	// Skip 'count' samples
	virtual void skip( long count );
	
	// True if a track was started and has since ended. Currently only dumped
	// format tracks (VGM, GYM) without loop points have an ending.
	bool track_ended() const;
	
	// Number of errors encountered while playing track due to undefined CPU
	// instructions in emulated formats and undefined stream events in
	// logged formats.
	int error_count() const;
	
protected:
	typedef BOOST::uint8_t byte; // used often
	int voice_count_;
	
	void set_track_count( int n ) { track_count_ = n; }
	void set_track_ended( bool b = true ) { track_ended_ = b; }
	void log_error() { error_count_++; }
private:
	// noncopyable
	Music_Emu( const Music_Emu& );
	Music_Emu& operator = ( const Music_Emu& );
	
	int mute_mask_;
	int error_count_;
	bool track_ended_;
	int track_count_;
};

inline int Music_Emu::error_count() const   { return error_count_; }
inline int Music_Emu::voice_count() const   { return voice_count_; }
inline int Music_Emu::track_count() const   { return track_count_; }
inline bool Music_Emu::track_ended() const  { return track_ended_; }

inline void Music_Emu::mute_voices( int mask ) { mute_mask_ = mask; }

inline void Music_Emu::start_track( int track )
{
	assert( (unsigned) track <= track_count() );
	track_ended_ = false;
	error_count_ = 0;
}

#endif


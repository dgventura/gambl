
// Game_Music_Box 0.5.2. http://www.slack.net/~ant/game-music-box

#include "File_Emu.h"

#include <cmath>
#include "thread_util.h"

#include "music_util.h"
#include "Spc_Emu.h"

/* Copyright (C) 2005 Shay Green. This module is free software; you
can redistribute it and/or modify it under the terms of the GNU Lesser
General Public License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version. This
library is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for
more details. You should have received a copy of the GNU Lesser General
Public License along with this module; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA */

#include "source_begin.h"

const int stereo = 2;
const int buf_msec = 1000 / 20; // size of sound buffer in milliseconds
const int silence_max = 6; // seconds
const int silence_threshold = 0x10;
const int fade_block_size = 512;
const int fade_length = 8; // seconds

File_Emu::File_Emu()
{
	mute_mask = 0;
	
	setup_.duration = 1.0;
	setup_.echo_depth = 0.2;
	setup_.treble = 0.5;
	setup_.bass = 0.5;
	setup_.custom_sound = false;
	
	effects_buffer.set_sample_rate( 44100, buf_msec ); // allocate now to avoid fragmentation
	stop();
}

File_Emu::~File_Emu() {
}

void File_Emu::stop()
{
	music_emu.reset();
	classic_emu = NULL;
	setup_changed = false;
	music_album = NULL;
	track_length_ = 0;
	sample_count_ = 0;
	sample_rate = 0;
	track = -1;
}

void File_Emu::load( Music_Album* album, long new_sample_rate )
{
	stop();
	sample_rate = new_sample_rate;
	
	// to do: remove once Vgm_Emu supports any rate
	effects_buffer.set_sample_rate( sample_rate, buf_msec );
	music_emu.reset( album->make_emu( sample_rate, &effects_buffer ) );
	if ( album->info().classic_emu )
	{
		// to do: uncomment once Vgm_Emu; supports any rate
		//effects_buffer.set_sample_rate( sample_rate, buf_msec );
		standard_eq = static_cast<Classic_Emu&> (*music_emu).equalizer();
	}
	setup_changed = true;
	fade_factor = std::pow( 0.005, 1.0 / (sample_rate * stereo * fade_length) );
	music_album = album;
}

void File_Emu::set_mute( int mask )
{
	setup_changed = false;
	mute_mask = mask;
	setup_changed = true;
}

void File_Emu::start_track( int new_track )
{
	// allow extra silence at beginning of track (Zelda LA track 61 has 20 seconds silence!)
	last_sound = 15 * stereo * sample_rate;
	is_done_ = false;
	sample_count_ = 0;
	track_extension = 0;
	fade_time = 0;
	
	bool already_loaded = (new_track == track);
	track = -1; // in case exception occurs
	int remapped = music_album->load_track( *music_emu, new_track, already_loaded );
	track = new_track;
	music_emu->start_track( remapped );
	if ( mute_mask )
		music_emu->mute_voices( mute_mask );
	if ( !music_album->info().classic_emu )
	{
		classic_emu = NULL;
	}
	else if ( !classic_emu )
	{
		classic_emu = static_cast<Classic_Emu*> (music_emu.get());
		setup_changed = true;
	}
	
	update_length();
}

void File_Emu::skip( long count )
{
	sample_count_ += count;
	last_sound = sample_count_;
	music_emu->skip( count );
}

void File_Emu::extend_track( int seconds )
{
	track_extension += seconds;
	update_length();
}

void File_Emu::update_length()
{
	int duration = music_album->info().duration;
	
	int fade_length = ::fade_length;
	double scale = setup_.duration;
	if ( duration < 0 )
	{
		fade_length = 0;
		duration = -duration;
		if ( duration > 20 ) {
			fade_length = 5;
			duration -= fade_length + 1;
		}
		if ( scale > 1.0 )
			scale = 1.0;
	}
	
	bool new_detect_silence = true;
	if ( !duration ) {
		duration = int (2 * 60 + 30 - fade_length);
	}
	else if ( scale <= 1.0 ) {
		new_detect_silence = false;
	}
	
	if ( duration > 45 || scale > 1.0 )
	{
		duration *= scale;
		if ( scale > 90 )
			duration = 60 * 60;
	}
	
	duration += track_extension;
	
	long new_fade_time = duration * stereo * sample_rate;
	
	// don't change fade time if already fading
	if ( new_fade_time != fade_time && sample_count_ <= fade_time )
	{
		if ( new_fade_time < sample_count_ )
			new_fade_time = sample_count_;
		fade_time = new_fade_time;
		track_length_ = duration + (duration > 10 ? fade_length : 0);
		detect_silence = new_detect_silence;
	}
}

void File_Emu::change_setup( const setup_t& new_setup )
{
	setup_changed = false; // don't let it read setup as it's being modified
	
	sync_memory();
	setup_ = new_setup;
	if ( music_album )
		update_length();
	sync_memory();
	
	setup_changed = true;
}

void File_Emu::apply_setup()
{
	if ( classic_emu )
	{
		Classic_Emu::equalizer_t eq = standard_eq;
		if ( setup_.custom_sound )
		{
			// bass - logarithmic, 2 to 8194 Hz
			double bass = 1.0 - setup_.bass;
			eq.bass = std::pow( 2.0, bass * 13 ) + 2.0;
			
			// treble - level from -108 to 0 to 5 dB
			double treble = setup_.treble * 2 - 1.0;
			
			//eq.cutoff = 0;
			//eq.treble = treble * (treble > 0 ? 16.0 : 80.0) - 8.0;
			
			// to do: which mapping to use?
			if ( treble > 0 ) {
				eq.treble = treble * 13.0 - 8.0;
				double c = 1.0 - treble;
				eq.cutoff = c * c * 6000;
			}
			else {
				treble *= 0.8;
				eq.treble = treble * 80 - 8.0;
				eq.cutoff = (1.0 + -treble * treble) * 6000;
			}
		}
		classic_emu->set_equalizer( eq );
		
		double depth = setup_.echo_depth;
		effects_config.pan_1 = -0.6 * depth;
		effects_config.pan_2 = 0.6 * depth;
		effects_config.reverb_delay = 880 * 0.1;
		effects_config.reverb_level = 0.5 * depth;
		effects_config.echo_delay = 610 * 0.1;
		effects_config.echo_level = 0.30 * depth;
		effects_config.delay_variance = 180 * 0.1;
		effects_config.effects_enabled = (setup_.custom_sound && setup_.echo_depth > 0.0);
		effects_buffer.config( effects_config );
	}
	music_emu->mute_voices( mute_mask );
}

static void scale_samples( BOOST::int16_t* p, int count, double fvol )
{
	int vol = fvol * 0x8000;
	while ( count-- ) {
		*p = (*p * vol) >> 15;
		p++;
	}
}

static bool is_silence( const BOOST::int16_t* p, int count )
{
	// Most of the time non-silence will be detected after only a few samples,
	// so efficiency isn't very important.
	int s0 = p [0];
	int s1 = p [1];
	for ( int n = count; n--; )
	{
		// handle one channel on each iteration, swapping variables each time
		int s = *p++;
		unsigned diff = s - s0 + silence_threshold / 2;
		s = s0;
		s0 = s1;
		s1 = s;
		if ( diff > silence_threshold )
			return false;
	}
	return true;
}

bool File_Emu::play( sample_t* buf, long count, bool force_check_silence )
{
	if ( track < 0 ) {
		is_done_ = true;
		return false;
	}
	
	// setup
	if ( setup_changed ) {
		setup_changed = false;
		apply_setup();
	}
	
	// play
	music_emu->play( count, buf );
	if ( music_emu->error_count() > 0 )
	{
		is_done_ = true;
		std::memset( buf, 0, count * sizeof *buf ); // emulation error
	}
	if ( music_emu->track_ended() )
		is_done_ = true;
	
	// silence
	bool silent = (force_check_silence ? is_silence( buf, count ) : false);
	if ( (!mute_mask && detect_silence) ||
			fade_time - sample_count_ < sample_rate * stereo * 2 )
	{
		if ( !force_check_silence )
			silent = is_silence( buf, count );
		
		if ( !silent )
			last_sound = sample_count_;
		
		if ( sample_count_ > last_sound + silence_max * stereo * sample_rate )
			is_done_ = true;
	}
	
	// fade
	if ( sample_count_ > fade_time )
	{
		for ( long i = 0; i < count; )
		{
			double level = std::pow( fade_factor, double (sample_count_ + i - fade_time) );
			if ( level < 0.005 )
				is_done_ = true;
			
			int n = count - i;
			if ( n > fade_block_size )
				n = fade_block_size;
			
			scale_samples( &buf [i], n, level );
			i += n;
		}
	}
	
	sample_count_ += count;
	
	return !(silent | is_done_);
}


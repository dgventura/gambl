
// Game_Music_Box 0.5.2. http://www.slack.net/~ant/game-music-box

#include "EmuInterface.h"

#include <cmath>

#include "sound_debugger.h"

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

// Record raw sound output to "debug.wav"
#define RECORD_SOUND 0

// Do sound synthesis in callback (can't use source-level debugger)
#define ASYNC_SOUND 0

const int max_blocks = (ASYNC_SOUND ? 12 : 30);
const int pending_block = max_blocks;

const int ffwd_blocks = 4;
const int stereo = 2;

EmuInterface::EmuInterface() :
	blocks( max_blocks + 1 )
{
	low_latency = false;
	mute_mask = 0;
	sample_rate = 44100;
	playing = false;
	silence_pending = 0;
	was_silent = false;
	silence_timeout = 0;
	
	ffwd_next_time = 0;
	fast_forwarding = false;
	
	write_pos = 0;
	read_pos = 0;
	
	deferred_active = false;
	deferred_enabled = false;
	
	blocks_played = 0;
	stop_output = false;
	
	#if RECORD_SOUND
		open_sound_debugger();
	#endif
}

EmuInterface::~EmuInterface()
{
	pause( false );
}

void EmuInterface::setup_changed( const setup_t& setup )
{
	emu.change_setup( setup );
	
	// volume (snap middle region to 1.0)
	double v = ((setup.volume - 0.5) * 0.8) + 0.5; // map left side to slightly non-zero
	v = v * v * 4.0; // exponential mapping
	if ( 0.85 < v && v < 1.15 ) // snap middle area to 1.0
		v = 1.0;
	player.set_gain( v );
}

void EmuInterface::set_mute( int mask )
{
	low_latency = (mask != 0);
	mute_mask = mask & 0x7fff;
	emu.set_mute( mute_mask );
}

const int fade_size = 512;

static void fade_samples( blip_sample_t* p, int step )
{
	const int shift = 15;
	int mul = (1 - step) << (shift - 1);
	step *= (1 << shift) / fade_size;
	
	for ( int n = fade_size; n--; )
	{
		*p = (*p * mul) >> 15;
		++p;
		mul += step;
	}
}

const int ramp_size = EmuInterface::block_size / 2;

static void make_ramp( blip_sample_t* p, int left, int right, int step )
{
	const int shift = 15;
	int mul = (1 - step) << (shift - 1);
	step *= (1 << shift) / ramp_size;
	
	for ( int n = ramp_size; n--; )
	{
		*p++ = (left * mul) >> shift;
		int temp = left;
		left = right;
		right = temp;
		mul += step;
	}
}

int EmuInterface::elapsed() const
{
	long n = emu.sample_count() - (filled_count() + silence_pending) *
			block_size;
	return n / (stereo * sample_rate);
}

bool EmuInterface::enable_fast_forward( bool b )
{
	bool result = fast_forwarding;
#if 0
	if ( !b )
	{
		//TODO: FF ffwd_timer.remove();
		fast_forwarding = false;
	}
	else if ( !ffwd_timer.installed() && emu.emu() )
	{
		ffwd_next_time = 0;
		//TODO: FF ffwd_timer.install( 10, 0.5 );
	}
#endif
	return result;
}

void EmuInterface::stop()
{
	pause( false );
	emu.stop();
}

bool EmuInterface::is_done( bool bCheckOnly ) const
{
	#if !ASYNC_SOUND
		if ( deferred_enabled && !fast_forwarding && !bCheckOnly )
		{
			for ( int n = max_blocks - 2 - filled_count(); n > 0; n -= 2 )
				const_cast<EmuInterface*> (this)->deferred_callback();
		}
	#endif
	
	return blocks_played > silence_timeout && emu.is_done();
}

void EmuInterface::play( Music_Album* album )
{
	pause( false );
	
	// Choose sample rate that's an integral division of hardware output rate,
	// and not greater than 56000 Hz.
	double rate = player.hw_sample_rate();
    assert( rate >= 11025 );
	for ( double divider = 1; divider <= 4; divider++ )
	{
		long r = rate / divider + 0.5;
		if ( r <= 56000 ) {
			sample_rate = r;
			break;
		}
	}
	
	sample_rate = album->sample_rate( sample_rate );
	player.setup( sample_rate, true, sound_callback_, this );
	emu.load( album, sample_rate );
}

void EmuInterface::start_track( int track )
{
	pause( false );
	emu.start_track( track );
	
	// clear queue
	write_pos = 0;
	read_pos = 0;
	
	// adjust fast forward state (fast-forward might be enabled)
	ffwd_next_time = 0;
	blocks_played = 0;
	
	// reset silence detection
	silence_pending = 0;
	was_silent = false;
	silence_timeout = INT_MAX;
}

void EmuInterface::stop_deferred()
{
	deferred_enabled = false;
	
	// deferred task might be active if running OS X
	do {
	}
	while ( deferred_active ); // to do: eliminate busy wait

    player.stop();
}

void EmuInterface::pause( bool might_resume )
{
	if ( !playing )
		return;
	
	stop_deferred();
	
	// fade audio out and wait for acknowledgement
	unsigned long timeout = TickCount() + 12;
	stop_output = true;
	do {
	}
	while ( stop_output && TickCount() < timeout );
	
	if ( stop_output )
	{
		// timed out; abruptly cut audio off
		check( false );
		stop_output = -1;
		player.stop();
	}
	
	playing = false;
	
	if ( might_resume )
	{
		// fill write queue to minimum if not already
		for ( int n = ffwd_blocks - filled_count(); --n >= 0; )
			queue_block();
		
		// fade first block in queue in so resume() won't click
		fade_samples( blocks [read_pos], 1 );
	}
}

void EmuInterface::play_buffer( const sample_t* p, long s )
{
	#if RECORD_SOUND
		write_sound_debugger( p, s );
	#endif
	player.play_buffer( p, s );
}

void EmuInterface::resume()
{
	if ( playing )
		return;
	
	assert( !deferred_active && !deferred_enabled );
	
	if ( emu.sample_count() == 0 )
	{
		// track was just started
		if ( !mute_mask )
		{
			// skip initial silence
			while ( !emu.play( blocks [pending_block], block_size, true ) && !emu.is_done() ) {
				// empty block
			}
			
			emu.reset_sample_count();
			
			silence_pending = 1; // pending block is filled
		}
		
		// fill write queue to minimum
		for ( int n = ASYNC_SOUND ? ffwd_blocks : max_blocks; n--; )
			queue_block();
	}
	
	deferred_enabled = true;
	stop_output = false;
	
	play_buffer( blocks [read_pos], block_size );
	playing = true;
}

// fast_forward

inline void EmuInterface::fast_forward()
{
	assert( playing );
	
	stop_deferred();
	fast_forwarding = true;
	
	queue_block();
	
	// trim excess blocks and fade last one out
	int remove_count = filled_count() - (ffwd_blocks + 2);
	if ( remove_count > 0 )
		write_pos = (write_pos + max_blocks - remove_count) % max_blocks;
	silence_pending = 0;
	fade_samples( blocks [(write_pos + max_blocks - 1) % max_blocks] +
			block_size - fade_size, -1 );
	
	// skip until maximum time is skipped or too much time has passed
	const int skip_len = 16;
	const int skip_samples = (sample_rate / skip_len) & ~1;
	for ( int n = 2 * skip_len; n-- && blocks_played <= ffwd_next_time; ) {
		emu.skip( skip_samples );
	}
	
	// wait for second block to begin
	while ( blocks_played <= ffwd_next_time ) // to do: eliminate busy wait
    {
    }
	ffwd_next_time = blocks_played + 1;
	
	// fade next block in
	queue_block();
	fade_samples( blocks [(write_pos + max_blocks - 1) % max_blocks], 1 );
	
	//TODO: FF ffwd_timer.set_next_time( 0.008 );
	deferred_enabled = true;
}

void EmuInterface::fast_forward_( void* self ) {
	static_cast<EmuInterface*> (self)->fast_forward();
}

// queue_block

int EmuInterface::filled_count() const
{
	return (write_pos + max_blocks - read_pos) % max_blocks;
}

void EmuInterface::queue_block()
{
	if ( filled_count() >= max_blocks - 1 )
		return; // no room in queue
	
	if ( (!silence_pending && emu.is_done()) )
		return; // track ended
	
	sample_t* buf = blocks [write_pos];
	
	if ( !silence_pending )
	{
		was_silent = !emu.play( buf, block_size, false );
	}
	else if ( --silence_pending == 0 )
	{
		// ran out of virtual silence blocks; add saved non-silent block
		std::memcpy( buf, blocks [pending_block], block_size * sizeof *buf );
	}
	else
	{
		// virtual silence blocks remaining
		
		assert( block_size >= ramp_size * 2 );
		
		int pos = 0;
		sample_t* prev = blocks [(write_pos + max_blocks - 1) % max_blocks];
		int prev_left = prev [block_size - 2];
		int prev_right = prev [block_size - 1];
		if ( prev_left | prev_right ) {
			make_ramp( buf, prev_left, prev_right, -1 );
			pos = ramp_size;
		}
		
		int end_ramp = 0;
		if ( silence_pending == 1 && !was_silent )
			end_ramp = ramp_size;
		std::memset( buf + pos, 0, (block_size - pos - end_ramp) * sizeof *buf );
		
		if ( end_ramp )
			make_ramp( buf + block_size - ramp_size, blocks [pending_block] [0],
					blocks [pending_block] [1], 1 );
	}
	
	if ( (was_silent || !silence_pending) && emu.is_done() )
	{
		// last block of track
		was_silent = false;
		fade_samples( buf + block_size - fade_size, -1 );
	}
	
	write_pos = (write_pos + 1) % max_blocks;
}

// deferred_callback

inline void EmuInterface::deferred_callback()
{
	check( !deferred_active );
	
	deferred_active = true;
	
	const volatile int f = false;
	scoped_restorer<volatile int> restorer( &deferred_active, f );
	
	if ( !deferred_enabled )
		return;
	
	if ( !emu.emu() ) {
		check( false );
		return;
	}
	
	if ( was_silent && !(fast_forwarding | mute_mask) )
	{
		// scan ahead for silence
		for ( int n = 3; n-- && was_silent; )
		{
			was_silent = !emu.play( blocks [pending_block], block_size, false );
			silence_pending++;
		}
	}
	
	// keep queue filled
	
	int count = ffwd_blocks - filled_count();
	if ( count < 2 && !low_latency && !fast_forwarding )
		count = 2;
	
	if ( !ASYNC_SOUND )
		count = 2;
	
	while ( --count >= 0 )
		queue_block();
	
	// update silence timeout
	if ( !was_silent && !emu.is_done() ) {
		silence_timeout = INT_MAX;
	}
	else if ( silence_timeout == INT_MAX )
	{
		int n = blocks_played + filled_count() + 2;
		if ( emu.is_done() )
			n += blocks_per_sec / 2;
		silence_timeout = n;
	}
}

void EmuInterface::deferred_callback_( void* data )
{
	static_cast<EmuInterface*> (data)->deferred_callback();
}

// sound_callback

inline void EmuInterface::sound_callback()
{
	blocks_played++;
	
	// advance to next block, or refill current with silence if queue is empty
	int filled = filled_count();
	if ( filled > 1 )
	{
		// to do: enable periodically under OS X to find if buffer is too small
		// (keeps tripping under OS 9, without any sound loss)
		//if ( filled == 2 )
		//  dprintf( "Sound queue is almost empty" );
		read_pos = (read_pos + 1) % max_blocks;
	}
	else {
		std::memset( blocks [read_pos], 0, block_size * sizeof (sample_t) );
	}
	
	int mode = stop_output;
	if ( !mode )
	{
		play_buffer( blocks [read_pos], block_size );
	//TODO: threading	if ( ASYNC_SOUND && dtask.install() )
	//		dprintf( "Deferred task was already installed" );
	}
	else if ( mode > 0 )
	{
		stop_output = -1;
		
		// Fade last block to avoid click. Reduce latency by fading at beginning
		// and discarding rest of block.
		// to do: discarding rest of block is slightly incorrect
		fade_samples( blocks [read_pos], -1 );
		play_buffer( blocks [read_pos], fade_size );
	}
	else
	{
		// faded-out block has been played
		stop_output = 0;
	}
    
#if 0
    printf( "MUSIC PLAYER %s: %d filled, read: %d write: %d played: %d\n", playing ? "PLAYING" : "STOPPED", filled, read_pos, write_pos, blocks_played );
#endif
}

void EmuInterface::sound_callback_( void* self )
{
	static_cast<EmuInterface*> (self)->sound_callback();
}

const short* EmuInterface::scope_buffer() const
{
	return (emu.emu() && emu.sample_count()) ? blocks [(read_pos + 1) % max_blocks] : NULL;
}


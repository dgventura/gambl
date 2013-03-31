
// Game_Music_Emu 0.2.6. http://www.slack.net/~ant/libs/

#include "Vgm_Emu.h"

#include <string.h>
#include <stdlib.h>
#include "blargg_endian.h"

/* Copyright (C) 2003-2005 Shay Green. This module is free software; you
can redistribute it and/or modify it under the terms of the GNU Lesser
General Public License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version. This
module is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for
more details. You should have received a copy of the GNU Lesser General
Public License along with this module; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA */

#include BLARGG_SOURCE_BEGIN

int const fir_size = 48;
double const gain = 1.5; // fir coeffs must match this

Vgm_Emu::Vgm_Emu()
{
	sample_buf = NULL;
}

Vgm_Emu::~Vgm_Emu()
{
	free( sample_buf );
}

const char** Vgm_Emu::voice_names() const
{
	static const char* names [] = {
		"FM 1", "FM 2", "FM 3", "FM 4", "FM 5", "FM 6", "PCM", "PSG"
	};
	if ( sample_buf )
		return names;
	return Vgm_Emu_Impl::voice_names();
}

void Vgm_Emu::update_eq( blip_eq_t const& eq )
{
	dac_synth.treble_eq( eq );
	Vgm_Emu_Impl::update_eq( eq );
}

void Vgm_Emu::mute_voices( int mask )
{
	if ( !sample_buf )
	{
		Vgm_Emu_Impl::mute_voices( mask );
	}
	else
	{
		Music_Emu::mute_voices( mask ); // bypass Classic_Emu
		psg.output( (mask & 0x80) ? NULL : blip_buf->center() );
		dac_synth.volume_unit( (mask & 0x40) ? 0.0 : 0.23 / 256 * gain );
		dac_synth.output( blip_buf->center() );
		ym2612_.mute_voices( mask );
	}
}

blargg_err_t Vgm_Emu::setup_fm()
{
	if ( !uses_fm_sound( header_ ) )
	{
		psg.volume( 1.0 );
		sample_buf_size = 0;
	}
	else
	{
		BLARGG_RETURN_ERR( init( vgm_rate ) ); // to do: use init() rate
		long ym2612_rate = get_le32( get_le32( header_.version ) >= 0x110 ?
				header_.ym2612_rate : header_.ym2413_rate );
		check( ym2612_rate );
		long fm_sample_rate = ym2612.fm_time( blip_buf->sample_rate() );
		BLARGG_RETURN_ERR( ym2612_.set_rate( fm_sample_rate,
				(double) ym2612_rate / fm_sample_rate ) );
		psg.volume( 0.27 * gain );
		buf_size = blip_buf->length() * blip_buf->sample_rate() / 1000;
		buf_size -= buf_size % fir_size;
		sample_buf_size = ym2612.fm_time( buf_size ) + fir_size;
		ym2612.emu = &ym2612_;
		voice_count_ = 8;
	}
	
	void* p = realloc( sample_buf, sample_buf_size * sizeof *sample_buf );
	BLARGG_CHECK_ALLOC( p || !sample_buf_size );
	sample_buf = (sample_pair_t*) p;
	
	return blargg_success;
}

void Vgm_Emu::start_track( int track )
{
	Vgm_Emu_Impl::start_track( track );
	
	if ( sample_buf )
	{
		memset( sample_buf [ym2612.fm_time( buf_size )], 0, fir_size * sizeof *sample_buf );
		ym2612_.reset();
		buf_pos = buf_size;
	}
}

void Ym_2612::run( int count )
{
	int const step = 6;
	int remain = count;
	do
	{
		int n = step;
		if ( n > remain )
			n = remain;
		remain -= n;
		emu::run_timer( n );
	}
	while ( remain > 0 );
	
	short* p = out;
	out += count * 2;
	emu::run( p, count * 2 );
}

void Vgm_Emu::write_pcm( blip_time_t time, int delta )
{
	if ( dac_amp >= 0 )
	{
		dac_amp += delta;
		dac_synth.offset_inline( time, delta );
	}
	else
	{
		dac_amp += delta;
		if ( !dac_enabled )
			dac_amp = -1;
	}
}

void Vgm_Emu::refill_buf( sample_t* out )
{
	memcpy( sample_buf [0], sample_buf [ym2612.fm_time( buf_size )],
			fir_size * sizeof *sample_buf );
	memset( sample_buf [fir_size], 0,
			(sample_buf_size - fir_size) * sizeof *sample_buf );
	
	ym2612_.out = sample_buf [fir_size];
	ym2612.begin_frame();
	run_commands( buf_size );
	assert( ym2612_.out == sample_buf [ym2612.fm_time( buf_size ) + fir_size] );
	
	blip_time_t blip_end = blip_buf->center()->count_clocks( buf_size );
	psg.end_frame( blip_end );
	blip_buf->center()->end_frame( blip_end );
	
	mix_samples( out );
	blip_buf->center()->remove_samples( buf_size );
	
	buf_pos = 0;
}

inline int Vgm_Emu::read_samples( int count, sample_t* out )
{
	int n = (buf_size - buf_pos) * 2;
	if ( n > count )
		n = count;
	memcpy( out, sample_buf [buf_pos], n * sizeof *out );
	buf_pos += n / 2;
	assert( buf_pos <= buf_size );
	return n;
}

void Vgm_Emu::play( long out_size, sample_t* out )
{
	require( pos ); // track must have been started
	
	if ( !sample_buf )
	{
		Vgm_Emu_Impl::play( out_size, out );
		return;
	}
	
	long out_pos = read_samples( out_size, out );
	
	while ( out_size - out_pos >= buf_size * 2 )
	{
		refill_buf( out + out_pos );
		out_pos += buf_size * 2;
		buf_pos = buf_size;
	}
	
	if ( out_pos < out_size )
	{
		refill_buf( sample_buf [0] );
		out_pos += read_samples( out_size - out_pos, out + out_pos );
	}
	assert( out_pos == out_size );
}

#include BLARGG_ENABLE_OPTIMIZER

void Vgm_Emu::mix_samples( sample_t* out )
{
	Blip_Reader sn;
	int bass = sn.begin( *blip_buf->center() );
	sample_t const* ym = sample_buf [0];
	
	// Use FIR to resample FM to 0.66 of its rate
	#define EVEN( i )   \
			((ym[i+0]*405 + ym[i+2]*1869 + ym[i+4]*7331 + ym[i+6]*11694 + \
			ym[i+8]*7331 + ym[i+10]*1869 + ym[i+12]*405 + ym[i+14]*39) >> 14)
			
	#define ODD( i )    \
			((ym[i+0]*180 + ym[i+2]*819 + ym[i+4]*4082 + ym[i+6]*10412 + \
			ym[i+8]*10412 + ym[i+10]*4082 + ym[i+12]*819 + ym[i+14]*180) >> 14)
	
	for ( int n = buf_size / 2; n--; )
	{
		int s1 = sn.read();
		sn.next( bass );
		long l1 = EVEN( 0 ) + s1;
		int s2 = sn.read();
		sn.next( bass );
		long l2 = ODD( 2 ) + s2;
		if ( (BOOST::int16_t) l1 != l1 )
			l1 = 0x7FFF - (l1 >> 24);
		long r1 = EVEN( 1 ) + s1;
		if ( (BOOST::int16_t) l2 != l2 )
			l2 = 0x7FFF - (l2 >> 24);
		long r2 = ODD( 3 ) + s2;
		if ( (BOOST::int16_t) r1 != r1 )
			r1 = 0x7FFF - (r1 >> 24);
		ym += 6;
		out [0] = l1;
		out [1] = r1;
		out [2] = l2;
		out [3] = r2;
		out += 4;
		if ( (BOOST::int16_t) r2 != r2 )
			out [-1] = 0x7FFF - (r2 >> 24);
	}
	
	sn.end( *blip_buf->center() );
}


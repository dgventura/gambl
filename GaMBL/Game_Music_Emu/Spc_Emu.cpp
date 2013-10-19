
// Game_Music_Emu 0.2.6. http://www.slack.net/~ant/libs/

#include "Spc_Emu.h"

#include <stdlib.h>
#include <string.h>
#include "blargg_endian.h"

/* Copyright (C) 2004-2005 Shay Green. This module is free software; you
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

Spc_Emu::Spc_Emu()
{
	resample_ratio = 1.0;
	use_resampler = false;
	spc_data = NULL;
    voice_count_ = Spc_Dsp::voice_count;
}

Spc_Emu::~Spc_Emu()
{
	free( spc_data );
}

const char** Spc_Emu::voice_names() const
{
	static const char* names [] = {
		"DSP 1", "DSP 2", "DSP 3", "DSP 4", "DSP 5", "DSP 6", "DSP 7", "DSP 8"
	};
	return names;
}

void Spc_Emu::mute_voices( int m )
{
	Music_Emu::mute_voices( m );
	apu.mute_voices( m );
}

blargg_err_t Spc_Emu::init( long sample_rate, double gain )
{
	apu.set_gain( gain );
	use_resampler = false;
	resample_ratio = (double) native_sample_rate / sample_rate;
	if ( sample_rate != native_sample_rate )
	{
		BLARGG_RETURN_ERR( resampler.buffer_size( native_sample_rate / 20 * 2 ) );
		resampler.time_ratio( resample_ratio, 0.9965 );
		use_resampler = true;
	}
	
	return blargg_success;
}

blargg_err_t Spc_Emu::load( Emu_Reader& in )
{
	header_t h;
	BLARGG_RETURN_ERR( in.read( &h, sizeof h ) );
	return load( h, in );
}

blargg_err_t Spc_Emu::load( const header_t& h, Emu_Reader& in )
{
	if ( in.remain() < Snes_Spc::spc_file_size - sizeof h )
		return "Not an SPC file";
	
	if ( strncmp( h.tag, "SNES-SPC700 Sound File Data", 27 ) != 0 )
		return "Not an SPC file";
	
	long remain = in.remain();
	long size = remain + sizeof h;
	if ( size < trailer_offset )
		size = trailer_offset;
	void* p = realloc( spc_data, size );
	BLARGG_CHECK_ALLOC( p );
	spc_data = (byte*) p;
	spc_data_size = size;
	
	set_track_count( 1 );
	voice_count_ = Snes_Spc::voice_count;
	
	memcpy( spc_data, &h, sizeof h );
	return in.read( spc_data + sizeof h, remain );
}

void Spc_Emu::start_track( int track )
{
	Music_Emu::start_track( track );
	
	resampler.clear();
	const char* error = apu.load_spc( spc_data, spc_data_size );
	check( !error );
}

void Spc_Emu::skip( long count )
{
	count = long (count * resample_ratio) & ~1;
	
	count -= resampler.skip_input( count );
	if ( count > 0 )
		apu.skip( count );
	
	// eliminate pop due to resampler
	const int resampler_latency = 64;
	sample_t buf [resampler_latency];
	play( resampler_latency, buf );
}

void Spc_Emu::play( long count, sample_t* out )
{
	require( track_count() ); // file must be loaded
	
	if ( !use_resampler )
	{
		apu.play( count, out );
		return;
	}
	
	long remain = count;
	while ( remain > 0 )
	{
		remain -= resampler.read( &out [count - remain], remain );
		if ( remain > 0 )
		{
			long n = resampler.max_write();
			if ( apu.play( n, resampler.buffer() ) )
				log_error();
			resampler.write( n );
		}
	}
	
	assert( remain == 0 );
}


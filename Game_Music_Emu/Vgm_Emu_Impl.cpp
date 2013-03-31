
// Game_Music_Emu 0.2.6. http://www.slack.net/~ant/libs/

#include "Vgm_Emu_Impl.h"

#include <math.h>
#include <string.h>
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

int const psg_time_bits = 12;
long const psg_time_unit = 1L << psg_time_bits;

enum {
	cmd_gg_stereo       = 0x4F,
	cmd_psg             = 0x50,
	cmd_ym2612_port0    = 0x52,
	cmd_ym2612_port1    = 0x53,
	cmd_delay           = 0x61,
	cmd_delay_735       = 0x62,
	cmd_delay_882       = 0x63,
	cmd_byte_delay      = 0x64,
	cmd_end             = 0x66,
	cmd_data_block      = 0x67,
	cmd_short_delay     = 0x70,
	cmd_pcm_delay       = 0x80,
	cmd_pcm_seek        = 0xE0,
	
	pcm_block_type      = 0x00,
	ym2612_dac_port     = 0x2A
};

inline long Vgm_Emu_Impl::to_psg_time( vgm_time_t time ) const
{
	return (time * psg_factor + psg_time_unit / 2) >> psg_time_bits;
}

Vgm_Emu_Impl::Vgm_Emu_Impl()
{
	pos = NULL;
	data = NULL;
	blip_buf = NULL;
	owned_data = NULL;
	
	// to do: decide on equalization parameters
	set_equalizer( equalizer_t( -32, 8000, 66 ) );
	psg.volume( 1.0 );
}

Vgm_Emu_Impl::~Vgm_Emu_Impl()
{
	delete blip_buf;
	unload();
}

void Vgm_Emu_Impl::unload()
{
	delete [] owned_data;
	owned_data = NULL;
	data = NULL;
	pos = NULL;
	set_track_ended( false );
}

blargg_err_t Vgm_Emu_Impl::init( long sample_rate )
{
	require( sample_rate == vgm_rate ); // to do: allow any rate
	if ( !blip_buf )
		BLARGG_CHECK_ALLOC( blip_buf = new Stereo_Buffer );
	
	BLARGG_RETURN_ERR( blip_buf->set_sample_rate( sample_rate, 1000 / 30 ) );
	return Classic_Emu::init( blip_buf );
}

blargg_err_t Vgm_Emu_Impl::init( Multi_Buffer* buf )
{
	require( buf->sample_rate() == vgm_rate ); // to do: allow any rate
	BLARGG_RETURN_ERR( buf->set_sample_rate( buf->sample_rate(), 1000 / 30 ) );
	return Classic_Emu::init( buf );
}

void Vgm_Emu_Impl::update_eq( blip_eq_t const& eq )
{
	psg.treble_eq( eq );
}

bool Vgm_Emu_Impl::uses_fm_sound( header_t const& h )
{
	if ( get_le32( h.ym2413_rate ) )
		return true;
	
	if ( get_le32( h.version ) < 0x110 )
		return false;
	
	if ( get_le32( h.ym2612_rate ) )
		return true;
	
	if ( get_le32( h.ym2151_rate ) )
		return true;
	
	return false;
}

BOOST::uint8_t const* Vgm_Emu_Impl::gd3_data( int* size ) const
{
	if ( size )
		*size = 0;
	
	long gd3_offset = get_le32( header_.gd3_offset );
	if ( !gd3_offset )
		return NULL;
	
	gd3_offset -= 0x40 - offsetof (header_t,gd3_offset);
	if ( gd3_offset < 0 )
		return NULL;
	
	byte const* gd3 = data + gd3_offset;
	long remain = data_end - gd3;
	if ( data_end - gd3 < 16 || 0 != memcmp( gd3, "Gd3 ", 4 ) || get_le32( gd3 + 4 ) >= 0x200 )
		return NULL;
	
	long gd3_size = get_le32( gd3 + 8 );
	if ( data_end - gd3 < gd3_size - 12 )
		return NULL;
	
	if ( size )
		*size = data_end - gd3;
	return gd3;
}

const char** Vgm_Emu_Impl::voice_names() const
{
	static const char* names [] = { "Square 1", "Square 2", "Square 3", "Noise" };
	return names;
}

void Vgm_Emu_Impl::set_voice( int i, Blip_Buffer* c, Blip_Buffer* l, Blip_Buffer* r )
{
	if ( i < psg.osc_count )
		psg.osc_output( i, c, l, r );
	else
		check( false );
}

blargg_err_t Vgm_Emu_Impl::setup_fm()
{
	if ( uses_fm_sound( header_ ) )
	{
		require( false ); // use Vgm_Emu_Impl::create() or Fm_Vgm_Emu for this VGM file
		return "FM sound requires FM VGM player";
	}
	return blargg_success;
}

blargg_err_t Vgm_Emu_Impl::load_( const header_t& h, void const* new_data, long new_size )
{
	header_ = h;
	
	// compatibility
	if ( 0 != memcmp( header_.signature, "Vgm ", 4 ) )
		return "Not a VGM file";
	check( get_le32( header_.version ) <= 0x150 );
	
	// psg rate
	long psg_rate = get_le32( header_.psg_rate );
	if ( !psg_rate )
		psg_rate = 3579545;
	psg_factor = (long) floor( (double) psg_time_unit / vgm_rate * psg_rate + 0.5 );
	
	data = (byte*) new_data;
	data_end = data + new_size;
	
	// get loop
	loop_begin = data_end;
	if ( get_le32( header_.loop_offset ) )
		loop_begin = &data [get_le32( header_.loop_offset ) + offsetof (header_t,loop_offset) - 0x40];
	
	voice_count_ = psg.osc_count;
	set_track_count( 1 );
	
	ym2612.emu = &null_ym;
	
	BLARGG_RETURN_ERR( setup_fm() );
	
	// do after FM in case output buffer is changed
	BLARGG_RETURN_ERR( Classic_Emu::setup_buffer( psg_rate ) );
	
	return blargg_success;
}

blargg_err_t Vgm_Emu_Impl::load( Data_Reader& reader )
{
	header_t h;
	BLARGG_RETURN_ERR( reader.read( &h, sizeof h ) );
	return load( h, reader );
}

blargg_err_t Vgm_Emu_Impl::load( const header_t& h, Data_Reader& reader )
{
	unload();
	
	// allocate and read data
	long data_size = reader.remain();
	int const padding = 8;
	BLARGG_CHECK_ALLOC( owned_data = BLARGG_NEW byte [data_size + padding] );
	blargg_err_t err = reader.read( owned_data, data_size );
	if ( err ) {
		unload();
		return err;
	}
	memset( owned_data + data_size, cmd_end, padding );
	
	return load_( h, owned_data, data_size );
}

void Vgm_Emu_Impl::start_track( int track )
{
	require( data ); // file must have been loaded
	
	Classic_Emu::start_track( track );
	psg.reset();
	
	dac_enabled = 0;
	pcm_data = data;
	pcm_pos = data;
	dac_amp = -1;
	vgm_time = 0;
	pos = data;
	if ( get_le32( header_.version ) >= 0x150 )
	{
		long data_offset = get_le32( header_.data_offset );
		check( data_offset );
		if ( data_offset )
			pos += data_offset + offsetof (header_t,data_offset) - 0x40;
	}
}

void Vgm_Emu_Impl::run_commands( vgm_time_t end_time )
{
	vgm_time_t vgm_time = this->vgm_time; 
	byte const* pos = this->pos;
	while ( vgm_time < end_time && pos < data_end )
	{
		switch ( *pos++ )
		{
		case cmd_end:
			pos = loop_begin;
			break;
		
		case cmd_delay_735:
			vgm_time += 735;
			break;
		
		case cmd_delay_882:
			vgm_time += 882;
			break;
		
		case cmd_gg_stereo:
			psg.write_ggstereo( to_psg_time( vgm_time ), *pos++ );
			break;
		
		case cmd_psg:
			psg.write_data( to_psg_time( vgm_time ), *pos++ );
			break;
		
		case cmd_delay:
			vgm_time += pos [1] * 0x100L + pos [0];
			pos += 2;
			break;
		
		case cmd_byte_delay:
			vgm_time += *pos++;
			break;
		
		case cmd_ym2612_port0:
			if ( pos [0] == ym2612_dac_port )
			{
				write_pcm( to_psg_time( vgm_time ), pos [1] - dac_amp );
			}
			else
			{
				ym2612.run_until( vgm_time );
				if ( pos [0] == 0x2B )
				{
					dac_enabled = pos [1] & 0x80;
					if ( !dac_enabled )
						dac_amp = -1;
				}
				ym2612.emu->write( pos [0], pos [1] );
			}
			pos += 2;
			break;
		
		case cmd_ym2612_port1:
			ym2612.run_until( vgm_time );
			ym2612.emu->write1( pos [0], pos [1] );
			pos += 2;
			break;
			
		case cmd_data_block: {
			check( *pos == cmd_end );
			int type = pos [1];
			long size = get_le32( pos + 2 );
			pos += 6;
			if ( type == pcm_block_type )
				pcm_data = pos;
			pos += size;
			break;
		}
		
		case cmd_pcm_seek:
			pcm_pos = pcm_data + pos [3] * 0x1000000L + pos [2] * 0x10000L +
					pos [1] * 0x100L + pos [0];
			pos += 4;
			break;
		
		// to do: skip unrecognized commands?
		default:
			int cmd = pos [-1];
			if ( (cmd & 0xf0) == cmd_pcm_delay )
			{
				vgm_time += cmd & 0x0f;
				write_pcm( to_psg_time( vgm_time ), *pcm_pos++ - dac_amp );
			}
			else if ( (cmd & 0xf0) == cmd_short_delay )
			{
				vgm_time += (cmd & 0x0f) + 1;
			}
			else if ( (cmd & 0xf0) == 0x50 )
			{
				pos += 2;
			}
			else
			{
				log_error();
			}
			break;
		}
	}
	this->pos = pos;
	vgm_time -= end_time;
	if ( vgm_time < 0 )
	{
		check( false );
		vgm_time = 0;
	}
	this->vgm_time = vgm_time;
	
	if ( pos >= data_end )
	{
		vgm_time = 0;
		set_track_ended();
		if ( pos > data_end )
			log_error();
	}
	ym2612.run_until( end_time );
}

long Vgm_Emu_Impl::run( int msec, bool* added_stereo )
{
	vgm_time_t end = msec * vgm_rate / 1000;
	ym2612.disable();
	
	run_commands( end );
	
	blip_time_t psg_end = to_psg_time( end );
	*added_stereo = psg.end_frame( psg_end );
	return psg_end;
}


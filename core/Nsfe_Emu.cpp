
// Game_Music_Emu 0.2.6. http://www.slack.net/~ant/libs/

#include "Nsfe_Emu.h"

#include <string.h>

/* Copyright (C) 2005 Shay Green. This module is free software; you
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

Nsfe_Emu::Nsfe_Emu( double gain ) : Nsf_Emu( gain )
{
}

Nsfe_Emu::~Nsfe_Emu()
{
}

const char* Nsfe_Emu::track_name( int i ) const
{
	if ( i < track_names.size() )
		return track_names [i];
	
	return "";
}

long Nsfe_Emu::track_time( int i ) const
{
	if ( i < track_times.size() )
		return track_times [i];
	
	return 0;
}

// Read little-endian 32-bit int
static blargg_err_t read_le32( Emu_Reader& in, long* out )
{
	unsigned char tag [4];
	BLARGG_RETURN_ERR( in.read( tag, sizeof tag ) );
	*out = tag [3] * 0x1000000L + tag [2] * 0x10000L + tag [1] * 0x100L + tag [0];
	return blargg_success;
}

// Read big-endian 32-bit int
static blargg_err_t read_be32( Emu_Reader& in, long* out )
{
	unsigned char tag [4];
	BLARGG_RETURN_ERR( in.read( tag, sizeof tag ) );
	*out = tag [0] * 0x1000000L + tag [1] * 0x10000L + tag [2] * 0x100L + tag [3];
	return blargg_success;
}

// Read multiple strings and separate into individual strings
static blargg_err_t read_strs( Emu_Reader& in, long size, std::vector<char>& chars,
		std::vector<const char*>& strs )
{
	chars.resize( size + 1 );
	chars [size] = 0; // in case last string doesn't have terminator
	BLARGG_RETURN_ERR( in.read( &(*chars.begin()), size ) );
	
	for ( int i = 0; i < size; i++ )
	{
        //TODO WARNING Not quite sure how this memory is being used, or why we are holding char pointers in a vector
        // as opposed to making a copy... seems a little dicey and need to come back to it later
        assert(0);
		strs.push_back( &(*chars.begin()) + i );
		while ( i < size && chars [i] )
			i++;
	}
	
	return blargg_success;
}

// Copy in to out, where out has out_max characters allocated. Truncate to
// out_max - 1 characters.
static void copy_str( const char* in, char* out, int out_max )
{
	out [out_max - 1] = 0;
	strncpy( out, in, out_max - 1 );
}

struct nsfe_info_t {
	unsigned char load_addr [2];
	unsigned char init_addr [2];
	unsigned char play_addr [2];
	unsigned char speed_flags;
	unsigned char chip_flags;
	unsigned char track_count;
	unsigned char first_track;
};
BOOST_STATIC_ASSERT( sizeof (nsfe_info_t) == 10 );

blargg_err_t Nsfe_Emu::load( Emu_Reader& in )
{
	header_t h;
	BLARGG_RETURN_ERR( in.read( &h, sizeof h ) );
	return load( h, in );
}

blargg_err_t Nsfe_Emu::load( const header_t& nsfe_tag, Emu_Reader& in )
{
	// check header
	if ( memcmp( nsfe_tag.tag, "NSFE", 4 ) )
		return "Not an NSFE file";
	
	// free previous info
	track_name_data.clear();
	track_names.clear();
	playlist.clear();
	track_times.clear();
	
	// default nsf header
	static const Nsf_Emu::header_t base_header =
	{
		'N','E','S','M','\x1A', // tag
		1,              // version
		1, 1,           // track count, first track
		0,0,0,0,0,0,    // addresses
		"","","",       // strings
		{ 0x1A, 0x41 }, // NTSC rate
		{ 0 },          // banks
		{ 0x20, 0x4E }, // PAL rate
		0,0,            // flags
		0,0,0,0         // unused
	};
	Nsf_Emu::header_t& header = info_;
	header = base_header;
	
	// parse tags
	int phase = 0;
	while ( phase != 3 )
	{
		// read size and tag
		long size = 0;
		long tag = 0;
		BLARGG_RETURN_ERR( read_le32( in, &size ) );
		BLARGG_RETURN_ERR( read_be32( in, &tag ) );
		
		switch ( tag )
		{
			case 'INFO': {
				check( phase == 0 );
				if ( size < 8 )
					return "Bad NSFE file";
				
				nsfe_info_t info;
				info.track_count = 1;
				info.first_track = 0;
				
				int s = size;
				if ( s > sizeof info )
					s = sizeof info;
				BLARGG_RETURN_ERR( in.read( &info, s ) );
				BLARGG_RETURN_ERR( in.skip( size - s ) );
				phase = 1;
				info_.speed_flags = info.speed_flags;
				info_.chip_flags = info.chip_flags;
				info_.track_count = info.track_count;
				info_.first_track = info.first_track;
				std::memcpy( info_.load_addr, info.load_addr, 2 * 3 );
				break;
			}
			
			case 'BANK':
				if ( size > sizeof info_.banks )
					return "Bad NSFE file";
				BLARGG_RETURN_ERR( in.read( info_.banks, size ) );
				break;
			
			case 'auth': {
				std::vector<char> chars;
				std::vector<const char*> strs;
				BLARGG_RETURN_ERR( read_strs( in, size, chars, strs ) );
				int n = strs.size();
				
				if ( n > 3 )
					copy_str( strs [3], info_.ripper, sizeof info_.ripper );
				
				if ( n > 2 )
					copy_str( strs [2], info_.copyright, sizeof info_.copyright );
				
				if ( n > 1 )
					copy_str( strs [1], info_.author, sizeof info_.author );
				
				if ( n > 0 )
					copy_str( strs [0], info_.game, sizeof info_.game );
				
				break;
			}
			
			case 'time': {
				track_times.resize( size / 4 );
				for ( int i = 0; i < track_times.size(); i++ )
					BLARGG_RETURN_ERR( read_le32( in, &track_times [i] ) );
				break;
			}
			
			case 'tlbl':
				BLARGG_RETURN_ERR( read_strs( in, size, track_name_data, track_names ) );
				break;
			
			case 'plst':
				playlist.resize( size );
				BLARGG_RETURN_ERR( in.read( &(*playlist.begin()), size ) );
				break;
			
			case 'DATA': {
				check( phase == 1 );
				phase = 2;
				Subset_Reader sub( &in, size ); // limit emu to nsf data
				BLARGG_RETURN_ERR( Nsf_Emu::load( info_, sub ) );
				check( sub.remain() == 0 );
				break;
			}
			
			case 'NEND':
				check( phase == 2 );
				phase = 3;
				break;
			
			default:
				// tags that can be skipped start with a lowercase character
				check( islower( (tag >> 24) & 0xff ) );
				BLARGG_RETURN_ERR( in.skip( size ) );
				break;
		}
	}
	
	return blargg_success;
}


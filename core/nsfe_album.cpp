
// Nintendo (NES) NSFE music file access with support for track tags, times, and
// playlist.

// Game_Music_Box 0.5.2. http://www.slack.net/~ant/game-music-box

#include "Music_Album.h"

#include "music_util.h"
#include "file_util.h"
#include "Nsf_Emu.h"

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

// NSFE

static long read_size( Emu_Reader& in ) {
	unsigned char tag [4];
	in.read( tag, sizeof tag );
	return tag [3] * 0x1000000 + tag [2] * 0x10000 + tag [1] * 0x100 + tag [0];
}

static long read_tag( Emu_Reader& in ) {
	unsigned char tag [4];
	in.read( tag, sizeof tag );
	return tag [0] * 0x1000000 + tag [1] * 0x10000 + tag [2] * 0x100 + tag [3];
}

static void read_strs( Emu_Reader& in, long size, runtime_array<char>& chars,
		runtime_array<const char*>& strs )
{
	chars.resize( size );
	in.read( chars.begin(), size );
	
	int count = 0;
	for ( int i = 0; i < size; i++ )
		if ( chars [i] == 0 )
			count++;
	
	strs.resize( count );
	const char* p = chars.begin();
	for ( int i = 0; i < count; i++ )
	{
		strs [i] = p;
		p += std::strlen( p ) + 1;
	}
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

struct Nsfe_Album : Music_Album
{
	runtime_array<unsigned char> track_map;
	runtime_array<const char*> track_names;
	runtime_array<char> track_name_data;
	runtime_array<int> track_durations;
	
	Music_Emu* make_emu( long, Multi_Buffer* buffer )
	{
		info_.classic_emu = true;
		info_.system = "Nintendo (NES)";
		
		BOOST::scoped_ptr<Nsf_Emu> emu( new Nsf_Emu );
		throw_if_error( emu->init( buffer ) );
		return emu.release();
	}
	
	int load_track_( Music_Emu& emu, int track, File_Archive* archive )
	{
		if ( archive )
		{
			BOOST::scoped_ptr<Emu_Reader> in( archive->extract() );
			
			track_map.clear();
			track_names.clear();
			track_name_data.clear();
			track_durations.clear();
			
			if ( read_tag( *in ) != 'NSFE' )
				throw_error( "Not an NSFE file" );
			
			static Nsf_Emu::header_t base_header = {
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
			
			Nsf_Emu::header_t header = base_header;
			
			int phase = 0;
			while ( phase != 3 )
			{
				long size = read_size( *in );
				long tag = read_tag( *in );
				switch ( tag )
				{
					case 'INFO': {
						check( phase == 0 );
						require( size >= 8 );
						
						nsfe_info_t info;
						info.track_count = 1;
						info.first_track = 0;
						
						int s = size;
						if ( s > sizeof info )
							s = sizeof info;
						in->read( &info, s );
						in->skip( size - s );
						phase = 1;
						header.speed_flags = info.speed_flags;
						header.chip_flags = info.chip_flags;
						header.track_count = info.track_count;
						header.first_track = info.first_track;
						std::memcpy( header.load_addr, info.load_addr, 2 * 3 );
						break;
					}
					
					case 'BANK':
						require( size <= sizeof header.banks );
						in->read( header.banks, size );
						break;
					
					case 'auth': {
						runtime_array<char> chars;
						runtime_array<const char*> strs;
						read_strs( *in, size, chars, strs );
						int n = strs.size();
						if ( n > 2 )
							set_field( info_.copyright, strs [2], 256 );
						if ( n > 1 )
							set_field( info_.author, strs [1], 256 );
						if ( n > 0 )
							set_field( info_.game, strs [0], 256 );
						break;
					}
					
					case 'time': {
						track_durations.resize( size / 4 );
						for ( int i = 0; i < track_durations.size(); i++ )
							track_durations [i] = (read_size( *in ) + 999) / 1000;
						break;
					}
					
					case 'tlbl':
						read_strs( *in, size, track_name_data, track_names );
						break;
					
					case 'plst':
						track_map.resize( size );
						in->read( track_map.begin(), size );
						break;
					
					case 'DATA': {
						check( phase == 1 );
						phase = 2;
						Subset_Reader sub( in.get(), size );
						throw_if_error( static_cast<Nsf_Emu&> (emu).load( header, sub ) );
						check( sub.remain() == 0 );
						break;
					}
					
					case 'NEND':
						check( phase == 2 );
						phase = 3;
						break;
					
					default:
						if ( isupper( (tag >> 24) & 0xff ) )
							throw_error( "NSFE requires a newer player version" );
						in->skip( size );
						break;
					
				}
			}
			
			// allow file CRC to be checked
			in->skip( in->remain() );
			
			info_.channel_names = static_cast<Nsf_Emu&> (emu).voice_names();
			
			if ( track_map.size() )
				set_track_count( track_map.size() );
		}
		
		if ( track < track_map.size() )
			track = track_map [track];
		
		info_.duration = 0;
		if ( track < track_durations.size() )
			info_.duration = track_durations [track];
		
		info_.song [0] = 0;
		if ( track < track_names.size() )
			set_field( info_.song, track_names [track], std::strlen( track_names [track] ) );
		
		info_changed();
		
		return track;
	}
	
	bool is_file_supported( const char* filename ) const {
		return has_extension( filename, ".NSFE" );
	}
};

Music_Album* new_nsfe_album() {
	return new Nsfe_Album();
}

int nsfe_track_count( File_Archive& archive )
{
	BOOST::scoped_ptr<Emu_Reader> in( archive.extract() );
	
	int count = 1;
	if ( read_tag( *in ) == 'NSFE' )
	{
		while ( true )
		{
			long size = read_size( *in );
			long tag = read_tag( *in );
			if ( tag == 'INFO' ) {
				nsfe_info_t info;
				info.track_count = 1;
				int s = size;
				if ( s > sizeof info )
					s = sizeof info;
				size -= s;
				in->read( &info, s );
				count = info.track_count;
			}
			else if ( tag == 'plst' ) {
				count = size;
				break;
			}
			else if ( tag == 'NEND' ) {
				break;
			}
			
			in->skip( size );
		}
	}
	
	return count;
}


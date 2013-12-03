
// Nintendo (NES) NSF, Game Boy GBS, Sega Master System VGM music file access.
// Supports extended VGM tags.

// Game_Music_Box 0.5.2. http://www.slack.net/~ant/game-music-box

#include "Music_Album.h"

#include "music_util.h"
#include "Gzip_Reader.h"
#include "file_util.h"
#include "Nsf_Emu.h"
#include "Gbs_Emu.h"
#include "Vgm_Emu.h"
#include "blargg_endian.h"

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

template<class Emu>
struct Classic_Album : Music_Album
{
	Vgm_Emu::equalizer_t default_eq;
	
	Music_Emu* make_emu( long, Multi_Buffer* buffer )
	{
		info_.classic_emu = true;
		unique_ptr<Emu> emu( new Emu );
		throw_if_error( emu->init( buffer ) );
		default_eq = emu->equalizer();
		return emu.release();
	}
	
	virtual void file_loaded( Emu&, const typename Emu::header_t& ) = 0;
	
	int load_track_( Music_Emu& music_emu, int track, File_Archive* archive )
	{
		if ( archive )
		{
			// to do: load data into memory for Mega Drive VGMs 
			Emu& emu = static_cast<Emu&> (music_emu);
			unique_ptr<Emu_Reader> in( archive->extract() );
			typename Emu::header_t header;
			throw_if_error( in->read( &header, sizeof header ) );
			
			// to do: this is really hacky
			// handle gzipped files in archive
			{
				runtime_array<char> ungzipped;
				if ( ((unsigned char*) &header) [0] == 0x1F &&
						((unsigned char*) &header) [1] == 0x8B )
				{
					// read gzipped file into memory
					runtime_array<char> gzipped( in->remain() + sizeof header );
					std::memcpy( gzipped.begin(), &header, sizeof header );
					throw_if_error( in->read( gzipped.begin() + sizeof header,
							gzipped.size() - sizeof header ) );
					
					inflate_mem_gzip( gzipped.begin(), gzipped.size(), ungzipped );
					in.reset( new Mem_File_Reader( ungzipped.begin(), ungzipped.size() ) );
					throw_if_error( in->read( &header, sizeof header ) );
				}
				
				throw_if_error( emu.load( header, *in ) );
				
				// allow file CRC to be checked
				in->skip( in->remain() );
			}
			
			info_.channel_names = emu.voice_names();
            info_.channel_count = emu.voice_count();
			file_loaded( emu, header );
			set_field( info_.author, header.author, sizeof header.author );
			set_field( info_.copyright, header.copyright, sizeof header.copyright );
			set_field( info_.game, header.game, sizeof header.game );
		}
		
		return track;
	}
};

// GBS

struct Gbs_Album : Classic_Album<Gbs_Emu>
{
	void file_loaded( Gbs_Emu&, const Gbs_Emu::header_t& ) {
		info_.system = "Game Boy";
	}
	
	bool is_file_supported( const char* filename ) const {
		return has_extension( filename, ".GBS" );
	}
};

Music_Album* new_gbs_album() {
	return new Gbs_Album();
}

int gbs_track_count( File_Archive& archive )
{
	Gbs_Emu::header_t h;
	archive.extract( &h, sizeof h );
	// check header to avoid getting a really large track count
	if ( identify_music_file_data( &h, archive.info().size ) == gbs_type )
		return h.track_count;
	return 0;
}

// NSF

static const char* nsf_system( const Nsf_Emu::header_t& h )
{
	// to do: decide whether to re-enable this extra info or remove this code
	// if re-enabling, nsfe_album.cpp also needs to call this
	//int f = h.chip_flags;
	//if ( f & 0x01 )
	//  return "Nintendo (NES) + Konami VRC6 Sound";
	//if ( f & 0x10 )
	//  return "Nintendo (NES) + Namco 106 Sound";
	return "Nintendo (NES)";
}

struct Nsf_Album : Classic_Album<Nsf_Emu>
{
	void file_loaded( Nsf_Emu&, const Nsf_Emu::header_t& header ) {
		info_.system = "Nintendo (NES)";
	}
	
	bool is_file_supported( const char* filename ) const {
		return has_extension( filename, ".NSF" );
	}
};

Music_Album* new_nsf_album() {
	return new Nsf_Album();
}

int nsf_track_count( File_Archive& archive )
{
	Nsf_Emu::header_t h;
	archive.extract( &h, sizeof h );
	// check header to avoid getting a really large track count
	// to do: keep synchronized with NSF emulator capability
	if ( identify_music_file_data( &h, archive.info().size ) == nsf_type &&
			h.vers == 1 && (h.chip_flags & ~0x31) == 0 )
		return h.track_count;
	return 0;
}

// VGM

typedef unsigned char byte;

static const byte* read_unistr_pair( const byte* in, char* out, int remain )
{
	remain--;
	while ( in [0] | in [1] )
	{
		if ( remain )
		{
			remain--;
			check( in [0] );
			*out++ = in [0];
		}
		in += 2;
	}
	*out = 0;
	in += 2;
	while ( in [0] | in [1] )
		in += 2;
	in += 2;
	return in;
}

struct Vgm_Album : Classic_Album<Vgm_Emu>
{
	char system [64];
	
	long sample_rate( long ) const { return 44100; } // to do: remove once Vgm_Emu supports others
	
	void file_loaded( Vgm_Emu& emu, const Vgm_Emu::header_t& header )
	{
		std::strcpy( system, "Master System" );
		info_.system = system;
		
		info_.classic_emu = !emu.uses_fm_sound( header );
		if ( !info_.classic_emu )
			emu.set_equalizer( default_eq ); // otherwise player won't restore default
		
		info_.duration = (long) get_le32( header.track_duration ) / -44100;
		if ( get_le32( header.loop_duration ) )
			info_.duration = 0;
		
		int gd3_size = 0;
		byte const* pos = emu.gd3_data( &gd3_size );
		if ( !gd3_size )
			return;
		
		static const char tag [8] = { 'G', 'd', '3', ' ', 0, 1, 0, 0 };
		if ( gd3_size < 16 || 0 != std::memcmp( pos, tag, sizeof tag ) ) {
			check( false );
			return;
		}
		const byte* end = pos + gd3_size;
		pos += sizeof tag;
		
		long claimed_size = get_le32( pos );
		pos += 4;
		if ( end - pos < claimed_size ) {
			check( false );
			return;
		}
		
		if ( pos >= end ) {
			check( false );
			return;
		}
		pos = read_unistr_pair( pos, info_.song, sizeof info_.song );
		
		if ( pos >= end ) {
			check( false );
			return;
		}
		pos = read_unistr_pair( pos, info_.game, sizeof info_.game );
		
		if ( pos >= end ) {
			check( false );
			return;
		}
		pos = read_unistr_pair( pos, system, sizeof system );
		
		if ( std::memcmp( "Sega ", system, 5 ) == 0 && system [5] != 'G' )
			std::memmove( system, system + 5, sizeof system - 5 );
		
		if ( pos >= end ) {
			check( false );
			return;
		}
		pos = read_unistr_pair( pos, info_.author, sizeof info_.author );
		if ( 0 == std::strcmp( info_.author, "Unknown" ) )
			info_.author [0] = 0;
		
		if ( pos >= end ) {
			check( false );
			return;
		}
		pos = read_unistr_pair( pos, info_.copyright, sizeof info_.copyright );
		if ( info_.copyright [4] == '/' )
			info_.copyright [4] = 0; // keep year only
		
		check( pos <= end );
	}
	
	bool is_file_supported( const char* filename ) const
	{
		return has_extension( filename, ".VGM" ) || has_extension( filename, ".VGZ" );
	}
};

Music_Album* new_vgm_album() {
	return new Vgm_Album();
}


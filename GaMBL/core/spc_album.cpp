
// Super Nintendo SPC music file access with extended tag support

// Game_Music_Box 0.5.2. http://www.slack.net/~ant/game-music-box

#include "Music_Album.h"

#include "music_util.h"
#include "FileUtilities.h"
#include "Spc_Emu.h"
#include "prefs.h"

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

static int spc_duration( const Spc_Emu::header_t& header )
{
	// duration
	char dstr [4];
	std::memcpy( dstr, header.len_secs, 3 );
	dstr [3] = 0;
	// SPC binary and text headers are quite difficult to differentiate
	if ( dstr [1] < 32 && !(dstr [1] == 0 && isdigit( dstr [0] )) ) { // binary
		return dstr [1] * 0x100 + dstr [0];
	}
	else {
		return std::atoi( dstr );
	}
	
	return 0;
}

struct Spc_Album : Music_Album
{
	Music_Emu* make_emu( long sample_rate, Multi_Buffer* )
	{
		info_.system = "Super Nintendo";
		unique_ptr<Spc_Emu> emu( new Spc_Emu );
		throw_if_error( emu->init( sample_rate ) );
		info_.channel_names = emu->voice_names();
        info_.channel_count = emu->voice_count();
		return emu.release();
	}
	
	typedef unsigned char byte;
	void parse_info( const Spc_Emu::header_t&, const byte* info, long size );
	
	int load_track_( Music_Emu& emu, int, File_Archive* archive )
	{
		if ( archive )
		{
			unique_ptr<Emu_Reader> in( archive->extract() );
			Spc_Emu::header_t header;
			throw_if_error( in->read( &header, sizeof header ) );
			Spc_Emu& spc_emu = static_cast<Spc_Emu&> (emu);
			throw_if_error( spc_emu.load( header, *in ) );
			if ( spc_emu.trailer_size() > 0 )
				parse_info( header, spc_emu.trailer(), spc_emu.trailer_size() );
		}
		
		static_cast<Spc_Emu&> (emu).disable_surround( prefs.disable_surround );
		
		return 0;
	}
	
	bool is_file_supported( const char* filename ) const
	{
		return has_extension( filename, ".SPC" );
	}
	
	int file_track_count() const
	{
		return 1;
	}
};

Music_Album* new_spc_album()
{
	return new Spc_Album();
}

struct Spcp_Album : Spc_Album
{
	GaMBLFileHandle path;
	
	Spcp_Album( const GaMBLFileHandle& path_ ) : path( path_ ) {
	}
	
	int load_track_( Music_Emu& emu, int, File_Archive* archive )
	{
		if ( archive )
		{
			Spc_Emu::header_t header;
			runtime_array<char> data;
			data.resize( archive->info().size );
			archive->extract( data.begin(), data.size() );
            std::wstring strPath;
            path.GetFilePath( strPath, true );
			unpack_spc( strPath, data );
			
			Emu_Mem_Reader in( data.begin(), data.size() );
			in.read( &header, sizeof header );
			Spc_Emu& spc_emu = static_cast<Spc_Emu&> (emu);
			throw_if_error( spc_emu.load( header, in ) );
			if ( spc_emu.trailer_size() > 0 )
				parse_info( header, spc_emu.trailer(), spc_emu.trailer_size() );
		}
		
		static_cast<Spc_Emu&> (emu).disable_surround( prefs.disable_surround );
		
		return 0;
	}
	
	bool is_file_supported( const char* filename ) const
	{
		return has_extension( filename, ".PSPC" );
	}
};

Music_Album* new_spcp_album( const GaMBLFileHandle& path )
{
	return new Spcp_Album( path );
}

static unsigned long get32( const unsigned char* p ) {
	return p [3] * 0x1000000L + p [2] * 0x10000L + p [1] * 0x100 + p [0];
}

void Spc_Album::parse_info( const Spc_Emu::header_t& header, const byte* pos, long size )
{
	const byte* end = pos + size;
	info_.duration = spc_duration( header );
	
	set_field( info_.game, header.game, sizeof header.game );
	set_field( info_.song, header.song, sizeof header.song );
	set_field( info_.author, header.author, sizeof header.author );
	
	char copyright [256];
	copyright [0] = 0;
	
	int year = 0;
	
	if ( end - pos < 8 )
		return;
	
	if ( 0 != std::memcmp( pos, "xid6", 4 ) ) {
		check( false );
		return; // I'm not aware of any other tag type
	}
	pos += 4;
	
	long info_size = get32( pos );
	pos += 4;
	
	// I'm not aware of files with data after header
	// (a couple from "Tengai Makyo Zero" have the IPL ROM appended after the tag)
	long max_size = end - pos;
	if ( max_size != info_size && max_size != info_size + 0x40 )
	{
		check( false ); // SPC xid6 tag length doesn't match end
		if ( info_size > max_size )
			info_size = max_size;
	}
	
	if ( end - pos < info_size )
		return;
	end = pos + info_size;
	
	int lengths [4] = { };
	int loop_count = 1;
	
	bool unpadded = false;
	
	while ( end - pos >= 4 )
	{
		int id = pos [0];
		int data = pos [3] * 0x100 + pos [2];
		int len = 0;
		int type = pos [1];
		if ( type != 0 )
			len = data;
		pos += 4;
		
		if ( end - pos < len ) {
			check( false );
			break; // SPC xid6 block went past end
		}
		
		switch ( id )
		{
			case 0x01:
				check( type == 1 );
				set_field( info_.song, (char*) pos, len );
				break;
			
			case 0x02:
				check( type == 1 );
				set_field( info_.game, (char*) pos, len );
				break;
			
			case 0x03:
				check( type == 1 );
				set_field( info_.author, (char*) pos, len );
				break;
			
			case 0x13:
				check( type == 1 );
				std::memcpy( copyright, (char*) pos, std::min( (int) sizeof copyright, len ) );
				break;
			
			case 0x14:
				check( type == 0 );
				year = data;
				break;
			
			case 0x30:
			case 0x31:
			case 0x32:
			case 0x33:
				check( type == 4 );
				lengths [id - 0x30] = get32( pos );
				break;
			
			case 0x34:
				check( type == 0 );
				if ( data != 0 )
					DPRINTF( "SPC mute: %X\n", (int) data );
				break;
			
			case 0x35:
				check( type == 0 );
				loop_count = data;
				break;
			
			case 0x36: {
				check( type == 4 );
				long gain = get32( pos );
				if ( gain && gain != 0x10000 )
					dprintf( "SPC gain: %f\n", gain / 65536.0 );
				break;
			}
			
			default:
				if ( id < 0x01 || (id > 0x07 && id < 0x10) || (id > 0x14 && id < 0x30) ||
						id > 0x36 )
					dprintf( "Unknown SPC xid6 block: %X\n", (int) id );
				break;
			
		}
		
		pos += len;
		
		const byte* unaligned = pos;
		
		// align to 4-byte boundary; padding should be zero
		while ( (end - pos) & 3 )
		{
			if ( *pos++ != 0 )
			{
				pos = unaligned;
				if ( !unpadded ) {
					unpadded = true;
					check( false );
				}
				break;
			}
		}
	}
	
	check( pos == end ); // shouldn't be data after tags
	
	// to do: use length information
	//if ( loop_count || lengths [0] || lengths [1] || lengths [2] || lengths [3] )
	//  debug_out << lengths [0] / 65536.0 << lengths [1] / 65536.0 <<
	//          lengths [2] / 65536.0 << lengths [3] / 65536.0 <<  loop_count;
	
	if ( year ) {
		char num [32];
		int len = snprintf( &num[0], sizeof(num), "%d ", year );
		std::memmove( copyright + len, copyright, sizeof copyright - len );
		std::memcpy( copyright, num, len );
	}
	copyright [sizeof copyright - 1] = 0;
	
	set_field( info_.copyright, copyright, sizeof copyright );
}



// Sega Genesis GYM music file access

// Game_Music_Box 0.5.2. http://www.slack.net/~ant/game-music-box

#include "Music_Album.h"

#include "file_util.h"
#include "Gym_Emu.h"

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

struct Gym_Album : Music_Album
{
	runtime_array<char> data;
	
	Music_Emu* make_emu( long sample_rate, Multi_Buffer* )
	{
		info_.system = "Sega Genesis";
		unique_ptr<Gym_Emu> emu( new Gym_Emu );
		throw_if_error( emu->init( sample_rate ) );
		return emu.release();
	}
	
	int load_track_( Music_Emu& music_emu, int, File_Archive* archive )
	{
		if ( archive )
		{
			Gym_Emu& emu = static_cast<Gym_Emu&> (music_emu);
			
			// avoid copying data
			data.clear();
			data.resize( archive->info().size );
			archive->extract( data.begin(), data.size() );
			throw_if_error( emu.load( data.begin(), data.size() ) );
			
			Gym_Emu::header_t& header = *(Gym_Emu::header_t*) data.begin();
			info_.duration = -emu.track_length();
			info_.channel_names = emu.voice_names();
            info_.channel_count = emu.voice_count();
			
			// be sure gym file has a header
			if ( std::memcmp( header.tag, "GYMX", 4 ) == 0 )
			{
				if ( 0 == std::strcmp( "Unknown Song", header.song ) )
					*header.song = 0;
				
				if ( 0 == std::strcmp( "Unknown Game", header.game ) )
					*header.game = 0;
				
				if ( 0 != std::strcmp( "Unknown Publisher", header.copyright ) )
					set_field( info_.copyright, header.copyright, sizeof header.copyright );
				
				set_field( info_.game, header.game, sizeof header.game );
				set_field( info_.song, header.song, sizeof header.song );
			}
		}
		
		return 0;
	}
	
	bool is_file_supported( const char* filename ) const
	{
		return has_extension( filename, ".GYM" );
	}
	
	int file_track_count() const
	{
		return 1;
	}
};

Music_Album* new_gym_album()
{
	return new Gym_Album();
}


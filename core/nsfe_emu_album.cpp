
// Nintendo (NES) NSFE music file access with support for track tags, times, and
// playlist.

// Game_Music_Box 0.5.2. http://www.slack.net/~ant/game-music-box

#include "Music_Album.h"

#include "file_util.h"
#include "Nsfe_Emu.h"

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

struct Nsfe_Album : Music_Album
{
	Music_Emu* make_emu( long, Multi_Buffer* buffer )
	{
		info_.classic_emu = true;
		info_.system = "Nintendo (NES)";
		
		unique_ptr<Nsfe_Emu> emu( new Nsfe_Emu );
		throw_if_error( emu->init( buffer ) );
		return emu.release();
	}
	
	int load_track_( Music_Emu& music_emu, int track, File_Archive* archive )
	{
		Nsfe_Emu& emu = static_cast<Nsfe_Emu&> (music_emu);
		
		if ( archive )
		{
			unique_ptr<Emu_Reader> in( archive->extract() );
			
			Nsfe_Emu::header_t header;
			throw_error( in->read( &header, sizeof header ) );
			throw_error( emu.load( header, *in ) );
			
			set_field( info_.copyright, emu.info().copyright, 256 );
			set_field( info_.author, emu.info().author, 256 );
			set_field( info_.game, emu.info().game, 256 );
			
			info_.channel_names = emu.voice_names();
			
			if ( emu.playlist_size() )
				set_track_count( emu.playlist_size() );
		}
		
		if ( track < emu.playlist_size() )
			track = emu.playlist_entry( track );
		
		info_.duration = (emu.track_time( track ) + 999) / 1000;
		
		set_field( info_.song, emu.track_name( track ), std::strlen( emu.track_name( track ) ) );
		
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
	unique_ptr<Emu_Reader> in( archive.extract() );
	
	Nsfe_Emu emu;
	throw_error( emu.init( 44100 ) );
	
	Nsfe_Emu::header_t header;
	throw_error( in->read( &header, sizeof header ) );
	throw_error( emu.load( header, *in ) );
	
	return emu.playlist_size() ? emu.playlist_size() : emu.track_count();
}


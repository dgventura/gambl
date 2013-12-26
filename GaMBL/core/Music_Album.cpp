
// Game_Music_Box 0.5.2. http://www.slack.net/~ant/game-music-box

#include <memory>

#include "Music_Album.h"

#include "music_util.h"
#include "FileUtilities.h"
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

// Mac OS X zip archives have hidden files starting with '.'
// and with normal file extensions.
static bool is_visible( const File_Archive::info_t& info )
{
	const char* str = std::strrchr( info.name, '/' );
	if ( str )
		str++;
	else
		str = info.name;
	return str [0] != '.';
}

// Music_Album

Music_Album::Music_Album()
{
	std::memset( &info_, 0, sizeof info_ );
}

void Music_Album::uncache()
{
	archive_->uncache();
}

void Music_Album::seek_archive( int track )
{
	if ( file_count > 1 )
	{
retry:
		if ( track >= file_count ) {
			debug_out << "Music_Album::seek_archive() track > file_count";
			check( false );
			track = 0;
		}
		
		if ( track <= current_track ) {
			current_track = -1;
			next_index = 0;
		}
		
		while ( archive_->seek( next_index, true ) )
		{
			next_index++;
			if ( archive_->info().is_file && is_visible( archive_->info() ) &&
					is_file_supported( archive_->info().name ) )
			{
				current_track++;
				if ( current_track == track )
					return;
			}
		}
		
		check( false );
		
		if ( track == 0 )
			throw_error( "Couldn't find track in archive" ); // give up
		
		// try again with first track
		track = 0;
		goto retry;
	}
	
	if ( !archive_->seek( next_index, true ) )
		throw_error( "Couldn't find track in archive" );
}

int Music_Album::load_track( Music_Emu& emu, int track, bool already_loaded )
{
	if ( !emu.track_count() || !already_loaded )
	{
		seek_archive( track );
		if ( file_count > 1 )
			track = 0;
		
		// clear fields
		info_.game [0] = 0;
		info_.song [0] = 0;
		info_.author [0] = 0;
		info_.copyright [0] = 0;
		std::strncpy( info_.filename, archive_->info().name, sizeof info_.filename );
		info_.filename [sizeof info_.filename - 1] = 0;
		remove_filename_extension( info_.filename );
		
		track = load_track_( emu, track, archive_.get() );
		
		// If this fails then we need to handle multi-track files
		// in multi-file archives.
		check( file_count == 1 || emu.track_count() == 1 );
		
		if ( file_count == 1 && !track_count_ )
			track_count_ = emu.track_count();
		
		// single file with no song name: use filename
		if ( emu.track_count() <= 1 && !*info_.song )
		 	std::strcpy( info_.song, info_.filename );
		
		if ( !*info_.game )
		{
			// no game name
			if ( emu.track_count() > 1 )
			{
				// multi-track: use filename
				std::strcpy( info_.game, info_.filename );
			}
			else
			{
				// single track: use name of parent folder
				if ( use_parent ) {
					archive_path = get_parent( archive_path );
					use_parent = false;
				}
				
				char parent_name [256];
				filename_without_extension( archive_path, parent_name );
				
				std::strncpy( info_.game, parent_name, sizeof info_.game );
				info_.game [sizeof info_.game - 1] = 0;
			}
		}
		
		// strip trailing space off game name
		int len = std::strlen( info_.game );
		while ( len && info_.game [len - 1] <= ' ' )
			len--;
		info_.game [len] = 0;
		
		info_changed();
	}
	else
	{
		if ( file_count > 1 )
			track = 0;
		
		track = load_track_( emu, track, NULL );
	}
	
	if ( track > emu.track_count() )
		track = 0;
	
	return track;
}

void Music_Album::set_field( char* out, const char* in, int len )
{
	// filter out lame "?" entries that should just be blank
	if ( *in && std::strcmp( in, "<?>" ) && std::strcmp( in, "< ? >" ) &&
			std::strcmp( in, "?" ) )
	{
		// remove copyright
		if ( 0 == std::memcmp( in, "(C) ", 4 ) || 0 == std::memcmp( in, "(c) ", 4 ) ) {
			in += 4;
			len -= 4;
		}
		if ( len >= max_field )
			len = max_field - 1;
		out [len] = 0;
		std::memcpy( out, in, len );
	}
}

void Music_Album::info_changed()
{
	// determine if song
	int d = info_.duration;
	if ( d < 0 )
		d = -d;
	info_.is_song = !d || d > 10;
}

void Music_Album::set_track_count( int n ) {
	if ( file_count == 1 )
		track_count_ = n;
}

// load_music_album

Music_Album* load_music_album( const GaMBLFileHandle& path )
{
	Cat_Info info;
	info.read( path );
	
	OSType type = identify_music_file( path, info.finfo().fileType );
	if ( !type )
		return NULL;
	
	return load_music_album( path, type );
}

Music_Album* load_music_album( const GaMBLFileHandle& fileHandle, OSType type )
{
	bool use_parent = false;
	
	int file_count = 0;
	int first_file = 0;
    
    std::wstring path;
    fileHandle.GetFilePath( path );
	
	unique_ptr<File_Archive> archive;
	if ( type == rar_type || type == zip_type )
	{
		archive.reset( type == rar_type ?
				open_rar_archive( path ) : open_zip_archive( path ) );
		type = 0;
		for ( int i = 0; archive->seek( i ); i++ )
		{
			if ( archive->info().is_file && is_visible( archive->info() ) )
			{
				OSType t = identify_music_filename( archive->info().name );
				if ( is_music_type( t ) )
				{
					if ( !type ) {
						type = t;
						first_file = i;
					}
					if ( type == t )
						file_count++;
				}
			}
		}
	}
	else {
		if ( type == gzip_type )
			type = identify_music_file_data( path );
		
		if ( type ) {
			use_parent = true;
			char str [256 + 8];
			archive.reset( open_file_archive( path, "" ) );
			file_count = 1;
		}
	}
	
	if ( !type )
		return NULL;
	
	Music_Album* album = NULL;
	switch ( type )
	{
		case nsf_type: album = new_nsf_album(); break;
		case nsfe_type:album = new_nsfe_album(); break;
		case gbs_type: album = new_gbs_album(); break;
		case vgm_type: album = new_vgm_album(); break;
		case gym_type: album = new_gym_album(); break;
		case spcp_type:album = new_spcp_album( path ); break;
		case spc_type: album = new_spc_album(); break;
	}
	if ( !album )
		return NULL;
	
	unique_ptr<Music_Album> owner( album );
	
	album->file_count = file_count;
	album->track_count_ = 0;
	if ( file_count > 1 )
		album->track_count_ = file_count;
	album->current_track = file_count; // not valid
	album->music_type_ = type;
	fileHandle.GetFilePath( album->archive_path );
	album->use_parent = use_parent;
	album->next_index = first_file;
	album->archive_.reset( archive.release() );
	return owner.release();
}

// album_track_count

static int album_track_count( OSType type, File_Archive& archive, int index )
{
	switch ( type )
	{
		case vgm_type:
		case gym_type:
		case spcp_type:
		case spc_type:
			return 1;
	}
	
	archive.seek( index, true );
	switch ( type )
	{
		case nsfe_type:return nsfe_track_count( archive );
		case nsf_type: return nsf_track_count( archive );
		case gbs_type: return gbs_track_count( archive );
	}
	
	assert( false );
	
	return 0;
}

int Music_Album::track_count()
{
	if ( !track_count_ )
	{
		assert( file_count == 1 );
		track_count_ = album_track_count( music_type_, *archive_, next_index );
	}
	return track_count_;
}

int album_track_count( const std::wstring& path, OSType type )
{
	int file_count = 0;
	int first_file = 0;
	
	unique_ptr<File_Archive> archive;
	if ( type == rar_type || type == zip_type )
	{
		archive.reset( type == rar_type ?
				open_rar_archive( path ) : open_zip_archive( path ) );
		type = 0;
		for ( int i = 0; archive->seek( i, false ); i++ )
		{
			if ( archive->info().is_file && is_visible( archive->info() ) )
			{
				OSType t = identify_music_filename( archive->info().name );
				if ( is_music_type( t ) )
				{
					if ( !type ) {
						type = t;
						first_file = i;
					}
					if ( type == t )
						file_count++;
				}
			}
		}
	}
	else {
		if ( type == gzip_type )
			type = identify_music_file_data( path );
		
		if ( type ) {
			archive.reset( open_file_archive( path, "" ) );
			file_count = 1;
		}
	}
	
	if ( !type )
		return 0;
	
	if ( file_count > 1 )
		return file_count;
	
	return album_track_count( type, *archive, first_file );
}



// Game music file utilities

// Game_Music_Box 0.5.2. Copyright (C) 2005 Shay Green. GNU LGPL license.

#ifndef MUSIC_UTIL_H
#define MUSIC_UTIL_H

#include <Foundation/Foundation.h>
#include "common.h"
#include "pod_vector.h"
class Cat_Info;

struct track_ref_t : GaMBLFileHandle {
	short track;
};

typedef pod_vector<track_ref_t> Music_Queue;

// Get track number from filename (#n in filename), or 0 if one isn't present.
int extract_track_num( std::wstring& filename );

// Append file or folder to playlist
void append_playlist( const GaMBLFileHandle&, Music_Queue& );

const OSType nsf_type = 'NSF ';
const OSType nsfe_type= 'NSFE';
const OSType gbs_type = 'GBS ';
const OSType vgm_type = 'VGM ';
const OSType gym_type = 'GYM ';
const OSType spc_type = 'SPC ';
const OSType spcp_type= 'PSPC'; // packed SPC

// Map type to music type above, or 0 if not a music type.
OSType is_music_type( OSType );

const OSType rar_type = 'RARf'; // RAR archive
const OSType zip_type = 'ZIP '; // ZIP archive
const OSType gzip_type= 'Gzip'; // gzipped file with file type set to 'Gzip'

// Map type to archive type above, or 0 if not an archive. Maps 'RAR ' to rar_type.
OSType is_archive_type( OSType );

// Map music/archive type, or 0 if not either.
OSType is_music_or_archive( OSType );

// True if file's type is "unset"
bool is_unset_file_type( OSType );

const OSType gmb_creator = 'VGMU';
const OSType shared_type = 'SDAT';

// File extension for music type, NULL if not a music type
const char* music_type_to_extension( OSType );

// Identify music file based on filename extension
OSType identify_music_filename( const char* );

// Determine if file is game music file, and what type it is. Result is 0
// if it's not a game music file. Providing type allows elimination of
// filesystem calls if file's type is already set correctly.
OSType identify_music_file( const GaMBLFileHandle& );
OSType identify_music_file( const GaMBLFileHandle&, OSType );
OSType identify_music_file( const HFSUniStr255&, OSType );

// Determine type of music file based on file's header. Result is 0 if unrecognized.
OSType identify_music_file_data( const std::wstring& strPath );
OSType identify_music_file_data( const void* first_four_bytes, int file_size );

// Read possibly gzipped and/or packed SPC file data into 'out'. False if
// SPC had no compression.
bool read_packed_spc( const std::wstring& strPath, runtime_array<char>& out );
bool unpack_spc( const GaMBLFileHandle& dir, runtime_array<char>& data );

// End of public interface
OSType identify_music_file_( const GaMBLFileHandle&, OSType );
inline OSType identify_music_file( const GaMBLFileHandle& path, OSType type ) {
	return identify_music_file_( path, type );
}
//inline OSType identify_music_file( const std::wstring& name, OSType type ) {
//	return identify_music_file_( NULL, type, &name );
//}
#endif


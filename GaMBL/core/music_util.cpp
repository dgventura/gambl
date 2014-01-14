
// Game_Music_Box 0.5.2. http://www.slack.net/~ant/game-music-box

#include "music_util.h"

#include <cctype>
#include <sys/fcntl.h>
#include <iostream>
#include <string.h>
#include "FileUtilities.h"
#include "Gzip_Reader.h"
#include "unpack_spc.h"
#include "Music_Album.h"

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

bool is_unset_file_type( OSType type )
{
	switch ( type )
	{
		case '\?\?\?\?':
		case 'TEXT':
		case 'BINA':
		case '    ':
		case 0:
			return true;
	}
	
	return false;
}

OSType is_music_type( OSType type )
{
	switch ( type )
	{
		case nsf_type:
		case nsfe_type:
		case gbs_type:
		case vgm_type:
		case gym_type:
		case spc_type:
		case spcp_type:
			return type;
	}
	
	return 0;
}

OSType is_archive_type( OSType type )
{
	switch ( type )
	{
		case 'RAR ':
			type = rar_type;
		case rar_type:
		case zip_type:
		case gzip_type:
			return type;
	}
	
	return 0;
}

OSType is_music_or_archive( OSType type )
{
	OSType result = is_music_type( type );
	if ( !result )
		result = is_archive_type( type );
	return result;
}

OSType identify_music_file_( const GaMBLFileHandle& fileHandle, OSType type )
{
/*	OSType result = is_music_or_archive( type );
	if ( result || !is_unset_file_type( type ) )
		return result;
/*
    std::wstring filename_;
	if ( !filename ) {
		assert( path );
		FSGetCatalogInfoChk( DeprecatedFSResolveAliasFileChk( *path ), 0, NULL, &filename_ ) ;
		filename = filename_;
	}
	char name [256];
	filename_to_str( *filename, name );
 */
    
    std::wstring path;
    fileHandle.GetFilePath( path, true );
    char szMbFilename[PATH_MAX];
    wcstombs( szMbFilename, path.c_str(), sizeof(szMbFilename) );
	return identify_music_filename( szMbFilename );
}

OSType identify_music_file( const GaMBLFileHandle& path )
{
	Cat_Info info;
	info.read( path );
	return identify_music_file( path, info.finfo().fileType );
}

const char* music_type_to_extension( OSType type )
{
	switch ( type )
	{
		case nsf_type: return ".NSF";
		case nsfe_type:return ".NSFE";
		case gbs_type: return ".GBS";
		case vgm_type: return ".VGM";
		case gym_type: return ".GYM";
		case spc_type: return ".SPC";
		case spcp_type:return ".PSPC";
	}
	
	return NULL;
}

OSType identify_music_filename( const char* name )
{
	long ext = 0;
	int len = std::strlen( name );
	if ( (len > 3 && name [len - 3] == '.') ||
			(len > 4 && name [len - 4] == '.') ||
			(len > 5 && name [len - 5] == '.') )
	{
		for ( int i = 0; i < 4; i++ )
			ext |= (std::toupper( name [len - i - 1] )) << (i * 8);
	}
	
	switch ( ext )
	{
		case '.NSF': return nsf_type;
		
		case 'NSFE': return nsfe_type;
		
		case '.GBS': return gbs_type;
		
		case '.VGM':
		case '.VGZ': return vgm_type;
		
		case '.GYM': return gym_type;
		case '.SPC': return spc_type;
		
		case '.RSN':
		case '.RAR': return rar_type;
		
		case '.ZIP': return zip_type;
	}
	
	if ( (ext & 0x00ffffff) == '.GZ' )
		return gzip_type;
	
	return 0;
}

OSType identify_music_file_data( const void* header, int size )
{
	const unsigned char* h = static_cast<const unsigned char*> (header);
	if ( size >= 4 )
	{
		switch ( h [0] * 0x1000000 + h [1] * 0x10000 + h [2] * 0x100 + h [3] )
		{
			case 'NESM':    return (size > 0x80) ? nsf_type : 0;
			case 'NSFE':    return (size > 100) ? nsfe_type : 0;
			case 'GBS\1':   return (size > 0x70) ? gbs_type : 0;
			case 'Vgm ':    return (size > 0x40) ? vgm_type : 0;
			case 'GYMX':    return gym_type;
			case 'SNES':    return ((unsigned) size - 0x10100 < 0x1000) ? spc_type : 0;
			case 'Rar!':    return rar_type;
			case 'PK\3\4':  return zip_type;
		}
	}
	
	return 0;
}

OSType identify_music_file_data( const std::wstring& path )
{
	Gzip_Reader in( path );
	long size = in.remain();
	unsigned char buf [4];
	if ( size < sizeof buf )
		return 0;
	
	in.read( buf, sizeof buf );
	return identify_music_file_data( buf, size );
}

bool unpack_spc( const std::wstring& path, runtime_array<char>& out )
{
	const char* shared_filename = spc_shared_filename( out.begin(), out.size() );
	if ( !shared_filename )
		return false;
	
	// find shared data file
    std::wstring dirPath = get_parent( path );
    std::wstring filename;
	str_to_filename( shared_filename, filename );
	GaMBLFileHandle shared_path = create_file( dirPath, filename );
	if ( !shared_path.IsOk() )
		throw_file_error( "The shared data file is missing", path );
	
	// read shared data
    shared_path.GetFilePath( filename, true );
	Gzip_Reader shared_in( filename );
	runtime_array<char> shared( shared_in.remain() );
	shared_in.read( shared.begin(), shared.size() );
	
	// unpack
	runtime_array<char> temp( out.size() );
	temp.swap( out );
	long actual_size = out.size();
	throw_if_error( unpack_spc( temp.begin(), temp.size(),
			shared.begin(), shared.size(), out.begin(), &actual_size ) );
	require( actual_size == out.size() );
	
	return true;
}
	
bool read_packed_spc( const std::wstring& path, runtime_array<char>& out )
{
	Gzip_Reader in( path );
	out.resize( in.remain() );
	in.read( out.begin(), out.size() );
	
	return unpack_spc( path, out ) || in.is_deflated();
}

int extract_track_num( std::wstring& name )
{
    //assert(0); // TEST
    std::wcout << name << std::endl;
    
	// to do: proper unicode access
	
	// first check at beginning of name, then scan backwards from end
	int len = name.length();
	int i = 0;
	do
	{
		if ( name[i] == L'#' )
		{
			wchar_t num [3];
			num [0] = name[i + 1];
			num [1] = (i + 2 < len ? name[i + 2] : 0);
			num [2] = 0;
			if ( i > 0 && name[i - 1] == L' ' )
				i--;
			return num[0] - L'0';
		}
		
		if ( i == 0 )
			i = len - 1;
        
        name.pop_back();
	}
	while ( --i );
    
    std::wcout << name << std::endl;
	
	return 0;
}

static void append_playlist_( const Cat_Info& info, Music_Queue& queue, int depth )
{
    std::wstring strAlbumPath;
    info.ref().GetFilePath( strAlbumPath, true );
    
#error aliases aren't being handled properly, shouldn't get down beyond this block
	if ( !info.is_dir() /*&& !is_dir_type( info.finfo().fileType )*/ )
	{
		OSType type = identify_music_file( info.ref(), info.finfo().fileType );
		if ( type )
		{
			track_ref_t tr;
			static_cast<GaMBLFileHandle&> (tr) = info.ref();
			
			int single = (info.is_alias() ? extract_track_num( strAlbumPath ) : 0);
			if ( single ) {
				tr.track = single - 1;
				queue.push_back( tr );
			}
			else
            {
				int track_count = album_track_count( strAlbumPath, type );
				for ( int i = 0; i < track_count; i++ )
                {
					tr.track = i;
					queue.push_back( tr );
				}
			}
			
			return; // handled
		}
	}
	
    GaMBLFileHandle dir = info.ref();
	if ( !info.is_dir() )
	{
		if ( !info.is_alias() )
			return;
  
        //TODO: do we need this?  I guess a directory alias?
//		Boolean is_dir = false, is_alias;
//		if ( FSResolveAliasFile( &dir, true, &is_dir, &is_alias ) || !is_dir )
//			return; // ignore error if alias couldn't be resolved
	}
	
	// directory
	if ( depth > 20 )
		throw_error( "Folders nested too deeply" );
	
    struct dirent *entry;
    DIR *dp;
    
    char szMbPath[PATH_MAX];
    wcstombs( szMbPath, strAlbumPath.c_str(), sizeof(szMbPath) );
    dp = opendir( szMbPath );
    if (dp == NULL) {
        assert(0);//("opendir: Path does not exist or could not be read.");
    }
    
    while ((entry = readdir(dp)))
    {
        GaMBLFileHandle newFile( entry->d_name, "r" );
        append_playlist( newFile, queue );
    }

    closedir( dp );
    
	/*HFSUniStr255 iter_name;
	for ( Dir_Iterator iter( dir ); iter.next( 0, &iter_name ); )
	{
		try {
			append_playlist_( iter, iter_name, queue, depth + 1 );
		}
		catch ( ... ) {
			// ignore errors
			check( false );
		}
	}*/
}

void append_playlist( const GaMBLFileHandle& path, Music_Queue& queue )
{
	Cat_Info info;
	info.read( path );
	append_playlist_( info, queue, 0 );
}

// Scan all files in archive and return common music type if there is one, -1 if
// there are multiple types, or 0 if there are no music files.
static OSType get_archive_type( const std::wstring& path, OSType type )
{
	unique_ptr<File_Archive> archive( type == zip_type ?
			open_zip_archive( path ) : open_rar_archive( path ) );
	type = 0;
	for ( int i = 0; archive->seek( i, false ); i++ )
	{
		if ( archive->info().is_file )
		{
			OSType item_type = identify_music_filename( archive->info().name );
			if ( item_type )
			{
				if ( !type )
					type = item_type;
				if ( item_type != type )
					return -1;
			}
		}
	}
	
	return type;
}


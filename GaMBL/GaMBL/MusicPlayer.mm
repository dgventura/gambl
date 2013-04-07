/*
 #import "MusicPlayer.h"


@implementation MusicPlayer

- (void)loadFile:(NSData *)dataBuffer
{
    track_ref_t& track_ref = current();
	
	OSType file_type = 0;
	
	Cat_Info info;
	
	// see if new track is in a different file than current one
	if ( !music_album || (0 != std::memcmp( &path, &album_path, sizeof album_path ) &&
                          0 != FSCompareFSRefs( &path, &album_path )) )
	{
		// current album might be bloated RAR archive
		uncache();
		
		info.read( track_ref, kFSCatInfoFinderInfo, dataBuffer->data );
		file_type = identify_music_file( path, info.finfo().fileType );
		if ( !file_type )
			return false;
		
		// won't load album if non-music file or archive with no music
		BOOST::scoped_ptr<Music_Album> album( load_music_album( path, file_type, name ) );
		if ( !album )
			return false;
		
		if ( file_type == gzip_type )
			file_type = album->music_type();
		
		player.pause( false ); // current album might hold data used by player
		music_album.reset(); // be sure current album is clear if error occurs
		
		player.play( album.get() );
		music_album.reset( album.release() );
		album_path = path;
	}
	
	if ( !start_track() )
		return false; // track needs to be skipped
	
	track_changed();
	
	// change file icon
	if ( prefs.change_icon && file_type )
	{
		if ( info.finfo().fileType != file_type || info.finfo().fileCreator != gmb_creator )
		{
			try {
				info.finfo().fileType = file_type;
				info.finfo().fileCreator = gmb_creator;
				info.write( path, kFSCatInfoFinderInfo );
			}
			catch ( ... ) {
				// ignore errors (might be write-protected)
				check( false );
			}
		}
	}
	
	return true;
}

@end
*/

//#include "MusicPlayer.h"
/*
void MusicPlayer::LoadFile( NSData* pDataBuffer )
{
    
}*/
//
//  AudioPlayer.cpp
//  GaMBL
//
//  Created by David Ventura on 4/6/13.
//  Copyright (c) 2013 David Ventura. All rights reserved.
//

#include <string>
#include "AudioPlayer.h"
#include "file_util.h"
#include "music_util.h"
#include "prefs.h"

extern App_Prefs prefs;
const int history_max = 2000;

AudioPlayer::AudioPlayer() : m_pMusicAlbum( NULL ), history_pos( -1 ), auto_unpause( false )
{
    prefs.init();
}

AudioPlayer::~AudioPlayer()
{
    
}

bool AudioPlayer::LoadFile( NSFileHandle* const pFile )
{
    char szBuf[PATH_MAX];
    int ret = fcntl( pFile.fileDescriptor, F_GETPATH, szBuf );
    assert( ret >= 0 );

    FSRef MyFileRef;
    OSStatus err = FSPathMakeRef( (UInt8*)szBuf, &MyFileRef, NULL );
    assert( err == noErr );
    
    // taken from begin_drop(), select_music_items()...
    reset_container( new_queue_ );
    append_playlist( MyFileRef, new_queue_ );
    
    end_drop();
    
    //play_current();
    
    
    return true;
}

void AudioPlayer::SetVolume( float fVolume )
{
    prefs.volume = fVolume;
    player.setup_changed( prefs );
}

shared_ptr< Music_Album > AudioPlayer::GetMusicAlbum() const
{
    return m_pMusicAlbum;
}

bool AudioPlayer::end_drop( bool immediate )
{
	if ( new_queue_.empty() )
		return false;
	
	if ( !m_pMusicAlbum || immediate )
	{
		queue.swap( new_queue_ );
		reset_container( new_queue_ );
		
		if ( history_pos + 1 == history.size() + 1 ) // avoid comparison of negative
			history_pos--; // was past end
		
		history.resize( history_pos + 1 );
		
		scoped_restorer<bool> r( &prefs.songs_only );
		if ( queue.size() == 1 )
			prefs.songs_only = false; // disable songs_only in case single non-song track was played
		next_track();
		if ( !playing )
			return false;
	}
	else
	{
		queue.insert( queue.end(), new_queue_.begin(), new_queue_.end() );
		reset_container( new_queue_ );
	}
	
	return true;
}

bool AudioPlayer::play_current()
{
//TODO    if ( scope && scope->visible() )
//		scope->clear( true );
    
    //TODO: figure out how behavior should work when playing to end
    	track_ref_t& track_ref = current();
    FSRef path = FSResolveAliasFileChk( track_ref );
    
	OSType file_type = 0;
	
	Cat_Info info;
	
	// see if new track is in a different file than current one
	if ( !m_pMusicAlbum || (0 != std::memcmp( &path, &album_path, sizeof album_path ) && 0 != FSCompareFSRefs( &path, &album_path )) )
	{
		// current album might be bloated RAR archive
		//uncache();
		
        HFSUniStr255 strUniFn;
        info.read( track_ref, kFSCatInfoFinderInfo, &strUniFn );
        
        file_type = identify_music_file( strUniFn, info.finfo().fileType );
        if ( !file_type )
            return false;
		
		// won't load album if non-music file or archive with no music
		shared_ptr<Music_Album> pNewAlbum( load_music_album( info.ref(), file_type, strUniFn ) );
		if ( !pNewAlbum )
			return false;
		
		if ( file_type == gzip_type )
			file_type = pNewAlbum->music_type();
		
        player.pause( false ); // current album might hold data used by player
        m_pMusicAlbum = NULL;
		
        player.play( pNewAlbum.get() ); //TODO: no naked pointers!
        m_pMusicAlbum = pNewAlbum;
        album_path = track_ref;
	}
	
    if ( !start_track() )
        return false; // track needs to be skipped
	
	return true;
}

bool AudioPlayer::next_track()
{
	history_pos++;
	
	while ( has_future() )
	{
		if ( play_current() )
			return true;
		
		// remove skipped track from history
		assert( history_pos < history.size() );
		history.erase( history.begin() + history_pos );
	}
	
	// no more tracks
	history_pos = history.size();
	stop();
	return false;
}

bool AudioPlayer::prev_track()
{
	if ( m_pMusicAlbum && (player.elapsed() > 3 || !playing) ) {
		if ( start_track() )
			return true;
	}
	
	while ( history_pos > 0 )
	{
		history_pos--;
		if ( !FSResolveAliasFileExists( current() ) ) {
			history.erase( history.begin() + history_pos );
		}
		else if ( play_current() )
			return true;
	}
	
	// no more tracks
	if ( history_pos != -1 ) {
		history_pos = -1;
		stop();
	}
	return false;
}

bool AudioPlayer::start_track()
{
	bool was_playing = playing;
	playing = false; // in case error occurs
	player.start_track( current().track );
	playing = was_playing;
	if ( prefs.songs_only && !m_pMusicAlbum->info().is_song )
		return false;
	
	if ( !auto_unpause ) {
		player.resume();
		playing = true;
		//TODO playback ui playing_changed();
	}
	//update_time();
	return true;
}

void AudioPlayer::stop( bool clear_history )
{
	if ( clear_history ) {
		reset_container( history );
		history_pos = -1;
		reset_container( queue );
	}
	
	assert( history_pos == -1 || history_pos == history.size() );
	if ( history_pos + 1 > history.size() + 1 ) // avoid comparing negatives
		history_pos = history.size();
	
	player.stop();
	
	stopped();
	
	// to do: return immediately if already stopped (otherwise ui flashes)
	
	m_pMusicAlbum.reset();
}

void AudioPlayer::stopped()
{
//TODO: ui 	enable_fast_forward( false );
	playing = false;
//TODO: ui 
//    prev_button.enable( history_pos > 0 );
//	next_button.enable( history_pos + 1 < history.size() );


#if 0
	playing_changed();
		RemoveWindowProxy( window() );
	std::memset( &proxy_path, 0, sizeof proxy_path );
	set_title( PROGRAM_NAME );
	track_text.set_text( "Drop game music here" );
	time_text.set_text( "" );
	author_text.set_text( "" );
	copyright_text.set_text( "" );
	system_text.set_text( "" );
	
	flush_window( window() ); // update UI as quickly as possible
	
	if ( scope )
		scope->clear();
	
	channels_window.enable_eq( true );
	channels_window.set_names( NULL, 0 );
	DisableMenuCommand( NULL, 'Expo' );
	DisableMenuCommand( NULL, 'ECur' );
	if ( dock_menu )
		SetMenuItemText( dock_menu, 1, "\pNot Playing" );

	track_timer.remove();
#endif
	reset_container( new_queue_ );
//TODO: ui	fast_forwarding = false;
	auto_unpause = false;
}

bool AudioPlayer::has_future()
{
	while ( history_pos < history.size() )
	{
		if ( FSResolveAliasFileExists( history [history_pos] ) )
			return true;
		history.erase( history.begin() + history_pos );
	}
	
	while ( !queue.empty() )
	{
		int index = (prefs.shuffle ? random( queue.size() ) : 0);
		track_ref_t track = queue [index];
		queue.erase( queue.begin() + index );
		if ( FSResolveAliasFileExists( track ) )
		{
			history.push_back( track );
			if ( history.size() > history_max ) {
				int n = history.size() - history_max;
				history_pos -= n;
				history.erase( history.begin(), history.begin() + n );
			}
			return true;
		}
	}

	return false;
}

track_ref_t& AudioPlayer::current() {
	assert( history_pos < history.size() );
	return history [history_pos];
}

static void append_time( string& strTemp, int seconds )
{
	char num [32];
    sprintf( num, "%d:%02d", seconds / 60, seconds % 60 );
    strTemp += num;
}

void AudioPlayer::update_time( string& strTemp )
{
    if ( player.is_done() )
    {
        next_track();
    }
    
	if ( !prefs.show_info || !m_pMusicAlbum )
		return;
	
	strTemp = "";
	append_time( strTemp, player.elapsed() );
	
	int duration = player.track_length();
	if ( duration < 60 * 60 ) {
		strTemp += " / ";
		append_time( strTemp, duration );
	}
    
    
    
    printf( "AUDIO PLAYER time: %s\n", strTemp.c_str() );
}
/*
void AudioPlayer::check_track_end()
{
	if ( auto_unpause ) {
		auto_unpause = false;
		if ( playing )
			player.resume();
	}
	
	if ( fast_forwarding | playing )
	{
		update_time();
		
		if ( player.is_done() )
		{
			try {
				next_track();
			}
			catch ( ... ) {
				report_exception();
			}
		}
	}
}*/
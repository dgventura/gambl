//
//  GameMusicPlayer.cpp
//  GaMBL
//
//  Created by David Ventura on 4/6/13.
//  Copyright (c) 2013 David Ventura. All rights reserved.
//

#include <string>
#include "GameMusicPlayer.h"
#include "FileUtilities.h"
#include "music_util.h"
#include "prefs.h"
#include "wave_export.h"

extern App_Prefs prefs;
const int history_max = 2000;

GameMusicPlayer::GameMusicPlayer() : m_pMusicAlbum( NULL ), history_pos( -1 ), auto_unpause( false ), playing( false )
{
    prefs.init();
}

GameMusicPlayer::~GameMusicPlayer()
{
    
}

bool GameMusicPlayer::LoadFile( NSFileHandle* const pFile )
{
    char szBuf[PATH_MAX];
    int ret = fcntl( pFile.fileDescriptor, F_GETPATH, szBuf );
    assert( ret >= 0 );
    std::wstring strPath(PATH_MAX);
    mbstowcs( strPath.begin(), szBuf, strlen(szBuf) );
    
    GaMBLFileHandle MyFileRef( strPath, "r" );
    assert( MyFileRef.IsOk() );
    
    // taken from begin_drop(), select_music_items()...
    reset_container( new_queue_ );
    append_playlist( MyFileRef, new_queue_ );
    
    end_drop();
    
    //play_current();
    
    
    return true;
}

void GameMusicPlayer::SetVolume( float fVolume )
{
    prefs.volume = fVolume;
    m_EmuInterface.setup_changed( prefs );
}

void GameMusicPlayer::SetChannelMask( unsigned int nMask )
{
    m_EmuInterface.set_mute( 0x8000 | nMask );
    m_EmuInterface.setup_changed( prefs );
}

void GameMusicPlayer::SetEqValues( bool bCustomSound, float fTreble, float fBass, float fStereo )
{
    prefs.custom_sound = bCustomSound;
    prefs.treble = fTreble;
    prefs.bass = fBass;
    prefs.echo_depth = fStereo;
    m_EmuInterface.setup_changed( prefs );
}

void GameMusicPlayer::SetShuffle( bool bShuffle )
{
    prefs.shuffle = bShuffle;
    m_EmuInterface.setup_changed( prefs );
}

void GameMusicPlayer::SetSkipShortTracks( bool bSkip )
{
    prefs.songs_only = bSkip;
    m_EmuInterface.setup_changed( prefs );
}

void GameMusicPlayer::SetPlayLength( int nLength )
{
    static float durations [] = { 1.0 / 2.73, 1.0, 2.0564, 100 };
	prefs.duration = durations [nLength];
    
    m_EmuInterface.setup_changed( prefs );
}

void GameMusicPlayer::ExtendCurrent()
{
    m_EmuInterface.extend_track( 60 * 1 );
}

bool GameMusicPlayer::PreviousTrackOk() const
{
    return history_pos > 0;
}

bool GameMusicPlayer::NextTrackOk()
{
    return history_pos + 1 < history.size() || has_future();
}

bool GameMusicPlayer::CurrentTrackOk() const
{
    return history_pos >= 0 && history_pos < history.size();;
}

shared_ptr< Music_Album > GameMusicPlayer::GetMusicAlbum() const
{
    return m_pMusicAlbum;
}

bool GameMusicPlayer::Playing() const
{
    return playing;
}

void GameMusicPlayer::RecordCurrentTrack( bool bSeparateAllChannels )
{
    record_track( GetCurrentTrack(), mute_mask, bSeparateAllChannels );
}

void GameMusicPlayer::record_track( const track_ref_t& track, int mute_mask, bool bSeparateAllChannels )
{
	// to do: make copy of track if use persists after nav dialog
	unique_ptr<Music_Album> album( load_music_album( DeprecatedFSResolveAliasFileChk( track ) ) );
	if ( !album )
    {
		check( false );
		return;
	}
	
	// open file in emulator and start track
	File_Emu emu;
	emu.change_setup( prefs );
	emu.load( album.get(), wave_sample_rate );
    emu.set_mute( mute_mask );
    emu.start_track( track.track ); // IMPORTANT!! This is needed to be used before grabbing album metadata because the starting actually loads the archive into memory
	
	// generate initial filename
	char name [256];
	std::strcpy( name, album->info().filename );
	if ( album->track_count() > 1 )
	{
		name [31 - 3] = 0;
		char num [32];
		num [0] = '-';
		num_to_str( track.track + 1, num + 1, 2 );
		std::strcat( name, num );
	}
	
	// get save location
	GaMBLFileHandle dir;
	HFSUniStr255 filename;
    
	if ( ask_save_file( &dir, &filename, name ) )
	{
        char baseFileName[PATH_MAX];
        filename_to_str( filename, baseFileName );
        remove_filename_extension( baseFileName );
        
        int nMaxChannels = bSeparateAllChannels ? emu.emu()->voice_count() : 1;

        for ( int nChannel = 0; nChannel < nMaxChannels; ++nChannel )
        {
            if ( bSeparateAllChannels )
            {
                char new_name [256];
                mute_mask = 0x7fff;
                mute_mask &= ~(1 << nChannel);
                sprintf( new_name, "%s %s.wav", baseFileName, album->info().channel_names[nChannel] );
                str_to_filename( new_name, filename );
            }
            
            emu.set_mute( mute_mask );
            emu.start_track( track.track );
            //album->uncache();
            
            try
            {
                write_wave( emu, dir, filename, -1, NULL/*&pw*/, bSeparateAllChannels );
            }
            catch ( ... )
            {
                handle_disk_full();
                throw;
            }
        }
	}
}

bool GameMusicPlayer::end_drop( bool immediate )
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
		PlayNextTrack();
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

bool GameMusicPlayer::play_current()
{
//TODO    if ( scope && scope->visible() )
//		scope->clear( true );
    
    //TODO: figure out how behavior should work when playing to end
    track_ref_t& track_ref = GetCurrentTrack();
    GaMBLFileHandle path = DeprecatedFSResolveAliasFileChk( track_ref );
    
	OSType file_type = 0;
	
	Cat_Info info;
	
	// see if new track is in a different file than current one
	if ( !m_pMusicAlbum || (0 != std::memcmp( &path, &album_path, sizeof album_path ) && 0 != DeprecatedFSCompareFSRefs( &path, &album_path )) )
	{		
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
		
        m_EmuInterface.pause( false ); // current album might hold data used by player
        m_pMusicAlbum = NULL;
		
        m_EmuInterface.play( pNewAlbum.get() ); //TODO: no naked pointers!
        m_pMusicAlbum = pNewAlbum;
        album_path = track_ref;
	}
	
    if ( !start_track() )
        return false; // track needs to be skipped
	
	return true;
}

bool GameMusicPlayer::PlayNextTrack()
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
	Stop();
	return false;
}

bool GameMusicPlayer::PlayPreviousTrack()
{
	if ( m_pMusicAlbum && (m_EmuInterface.elapsed() > 3 || !playing) ) {
		if ( start_track() )
			return true;
	}
	
	while ( history_pos > 0 )
	{
		history_pos--;
		if ( !FSResolveAliasFileExists( GetCurrentTrack() ) ) {
			history.erase( history.begin() + history_pos );
		}
		else if ( play_current() )
			return true;
	}
	
	// no more tracks
	if ( history_pos != -1 ) {
		history_pos = -1;
		Stop();
	}
	return false;
}

bool GameMusicPlayer::start_track()
{
	bool was_playing = playing;
	playing = false; // in case error occurs
	m_EmuInterface.start_track( GetCurrentTrack().track );
	playing = was_playing;
	if ( prefs.songs_only && !m_pMusicAlbum->info().is_song )
		return false;
	
	if ( !auto_unpause ) {
		m_EmuInterface.resume();
		playing = true;
		//TODO playback ui playing_changed();
	}
	//update_time();
	return true;
}

void GameMusicPlayer::Stop( bool clear_history )
{
	if ( clear_history ) {
		reset_container( history );
		history_pos = -1;
		reset_container( queue );
	}
	
	assert( history_pos == -1 || history_pos == history.size() );
	if ( history_pos + 1 > history.size() + 1 ) // avoid comparing negatives
		history_pos = history.size();
	
	m_EmuInterface.stop();
	
	stopped();
	
	// to do: return immediately if already stopped (otherwise ui flashes)
	
	m_pMusicAlbum.reset();
}

void GameMusicPlayer::TogglePause()
{
	playing = !playing;
	if ( playing ) {
		m_EmuInterface.resume();
	}
	else {
		m_EmuInterface.pause();
	}
}

void GameMusicPlayer::stopped()
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

bool GameMusicPlayer::has_future()
{
	while ( history_pos < history.size() )
	{
		if ( DeprecatedFSResolveAliasFileExists( history [history_pos] ) )
			return true;
		history.erase( history.begin() + history_pos );
	}
	
	while ( !queue.empty() )
	{
		int index = (prefs.shuffle ? random( queue.size() ) : 0);
		track_ref_t track = queue [index];
		queue.erase( queue.begin() + index );
		if ( DeprecatedFSResolveAliasFileExists( track ) )
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

track_ref_t& GameMusicPlayer::GetCurrentTrack() {
	assert( history_pos < history.size() );
	return history [history_pos];
}

static void append_time( string& strTemp, int seconds )
{
	char num [32];
    sprintf( num, "%d:%02d", seconds / 60, seconds % 60 );
    strTemp += num;
}

void GameMusicPlayer::UpdatePlaybackTime( string& strTemp )
{
    if ( m_EmuInterface.is_done( TRUE ) )
    {
        PlayNextTrack();
    }
    
	if ( !prefs.show_info || !m_pMusicAlbum )
		return;
	
	strTemp = "";
	append_time( strTemp, m_EmuInterface.elapsed() );
	
	int duration = m_EmuInterface.track_length();
	if ( duration < 60 * 60 ) {
		strTemp += " / ";
		append_time( strTemp, duration );
	}
    
#if GAMBL_VERBOSE
    printf( "AUDIO PLAYER time: %s\n", strTemp.c_str() );
#endif
}
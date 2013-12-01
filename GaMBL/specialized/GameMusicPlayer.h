//
//  GameMusicPlayer.h
//  GaMBL
//
//  Created by David Ventura on 4/6/13.
//  Copyright (c) 2013 David Ventura. All rights reserved.
//

#ifndef     MUSICPLAYER_H
#define     MUSICPLAYER_H

#import <Foundation/Foundation.h>
#include <stdio.h>
#include <memory>
#include "EmuInterface.h"
#include "Music_Album.h"
#include "music_util.h"

using namespace std;

class GameMusicPlayer
{
public:
    GameMusicPlayer();
    virtual ~GameMusicPlayer();

    bool LoadFile( NSFileHandle* const pDataBuffer );
    
    shared_ptr< Music_Album > GetMusicAlbum() const;
    
    // mutators for UI handling
    void SetVolume( float fVolume );
    void SetChannelMask( unsigned int nMask );
    void SetEqValues( bool bCustomSound, float fTreble, float fBass, float fStereo );
    void SetShuffle( bool bShuffle );
    void SetSkipShortTracks( bool bSkip );
    void SetPlayLength( int nLength );
    void ExtendCurrent();
    void Stop( bool clear_history = false );
    bool PlayPreviousTrack();
	bool PlayNextTrack();
	void TogglePause();
    void UpdatePlaybackTime( string& strTemp );
    void RecordCurrentTrack( bool bSeparateAllChannels );

    // query playback state
    bool NextTrackOk();
    bool PreviousTrackOk() const;
    bool CurrentTrackOk() const;
    bool Playing() const;
    track_ref_t& GetCurrentTrack();
    
private:    
    // incoming files
    Music_Queue new_queue_;
    bool end_drop( bool immediate = false );
    
    // queue
    Music_Queue queue;
	Music_Queue history;
	int history_pos;
    bool has_future();
    
    // player
    shared_ptr< Music_Album > m_pMusicAlbum;
    EmuInterface m_EmuInterface;
	FSRef album_path;
	int mute_mask;
	bool playing;
	bool start_track();
    void stopped();
    bool play_current();
    void record_track( const track_ref_t& track, int mute_mask, bool bSeparateAllChannels );
    
    bool auto_unpause;
};

#endif //   MUSICPLAYER_H

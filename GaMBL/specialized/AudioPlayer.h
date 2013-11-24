//
//  AudioPlayer.h
//  GaMBL
//
//  Created by David Ventura on 4/6/13.
//  Copyright (c) 2013 David Ventura. All rights reserved.
//

#ifndef     AUDIOPLAYER_H
#define     AUDIOPLAYER_H

#import <Foundation/Foundation.h>
#include <stdio.h>
#include <memory>
#include "Music_Player.h"
#include "Music_Album.h"
#include "music_util.h"

using namespace std;

class AudioPlayer
{
public:
    AudioPlayer();
    virtual ~AudioPlayer();

    bool LoadFile( NSFileHandle* const pDataBuffer );
    shared_ptr< Music_Album > GetMusicAlbum() const;
    bool play_current();
    
    void SetVolume( float fVolume );
    void SetChannelMask( unsigned int nMask );
    void SetEqValues( bool bCustomSound, float fTreble, float fBass, float fStereo );
    void SetShuffle( bool bShuffle );
    void SetSkipShortTracks( bool bSkip );
    void SetPlayLength( int nLength );
    void ExtendCurrent();
    void RecordCurrentTrack( bool bSeparateAllChannels );
    
    bool NextTrackOk();
    bool PreviousTrackOk() const;
    bool CurrentTrackOk() const;
    bool Playing() const;
    void stop( bool clear_history = false );
    track_ref_t& current();
    bool has_future();
    
    bool prev_track();
	bool next_track();
	void toggle_pause();
    void update_time( string& strTemp );

    
private:    
    // incoming files
    Music_Queue new_queue_;
	void queue_files( const FSRef&, const Cat_Info&, HFSUniStr255&,
                     Music_Queue&, int depth );
    bool end_drop( bool immediate = false );
    
    // queue
    Music_Queue queue;
	Music_Queue history;
	int history_pos;
    
    // player
    shared_ptr< Music_Album > m_pMusicAlbum;
    Music_Player player;
	FSRef album_path;
	int mute_mask;
	bool playing;
	bool play_current_();
	bool start_track();

    void stopped();
    
    void record_track( const track_ref_t& track, int mute_mask, bool bSeparateAllChannels );
    
    bool auto_unpause;
};

#endif //   AUDIOPLAYER_H

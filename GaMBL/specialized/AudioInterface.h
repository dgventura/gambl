//
//  AudioInterface.h
//  GaMBL
//
//  Created by David Ventura on 4/11/13.
//  Copyright (c) 2013 David Ventura. All rights reserved.
//
//  Note: this class is largely-based on Tim Bolstad's iOS CoreAudio tutorial.
//  Since it was originally written to provide an example of writing samples to
//  output buffer, it came pretty close to what I needed for GaMBL.  Thanks Tim!
//
//  http://timbolstad.com/2010/03/14/core-audio-getting-started/
//

#ifndef AUDIO_INTERFACE_H
#define AUDIO_INTERFACE_H

#include <memory>
#include <AudioToolbox/AudioToolbox.h>
#include "CAStreamBasicDescription.h"

class Music_Player;

class AudioInterface
{
public:
    AudioInterface();
    virtual ~AudioInterface();

    // legacy functions for GMB
    void set_gain( double dGain );
    double hw_sample_rate() const { return 44100; } // TODO: hw query
    typedef void (*callback_t)( void* );
    void setup( double nSampleRate, bool bStereo, callback_t func, void* pData = NULL );
    typedef short sample_t;
    void play_buffer( const sample_t* pSampleBuffer, int nCount );
    void stop();
    
    // Sine Wave Phase marker
    double sinPhase;
    
    //TODO: don't save these KARIKARI
    Music_Player* m_pMusicPlayer;
    const sample_t* m_pSampleHead;
    int m_nSampleCount;
    
private:
    // audio unit graph control
    void initializeAUGraph( double nSampleRate, bool bStereo );
    void startAUGraph();
    void stopAUGraph();
    
    // Audio Graph Members
    AUGraph   mGraph;
    AudioUnit mMixer;
    
    bool m_bPlaying;
    
    callback_t m_MusicPlayerCallback;

    // Audio Stream Descriptions
    CAStreamBasicDescription outputCASBD;
    
  
};

#endif /* defined(AUDIO_INTERFACE_H) */

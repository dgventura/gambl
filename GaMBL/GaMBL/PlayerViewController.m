//
//  PlayerViewController.m
//  GaMBL
//
//  Created by David Ventura on 4/28/13.
//  Copyright (c) 2013 David Ventura. All rights reserved.
//

#import "PlayerViewController.h"
#import "AppDelegate.h"
#include <string>

@implementation PlayerViewController

- (id)initWithNibName:(NSString *)nibNameOrNil bundle:(NSBundle *)nibBundleOrNil
{
    self = [super initWithNibName:nibNameOrNil bundle:nibBundleOrNil];
    if (self)
    {
        // start a timer to keep track of the playback status
        const float framerate = 2;
        const float frequency = 1.0f/framerate;
        _PlaybackTimer = [NSTimer scheduledTimerWithTimeInterval:frequency target:self selector:@selector(updatePlaybackUI) userInfo:nil repeats:YES];
    }
    
    return self;
}

- (void)updatePlaybackUI
{
    AppDelegate* pAppDelegate = (AppDelegate *)[NSApp delegate];
    GameMusicPlayer* pAI = [pAppDelegate musicPlayer];
    
    string strTemp;
    pAI->update_time( strTemp );
    NSString* pStrTemp = [[NSString alloc] initWithUTF8String:strTemp.c_str() ];
    [_playbackTimeLabel setStringValue:pStrTemp];
    
    // playback buttons
    [_nextButton setEnabled:pAI->NextTrackOk()];
    [_previousButton setEnabled:pAI->PreviousTrackOk()];
    [_playButton setEnabled:pAI->CurrentTrackOk()];
    
    [self updateTrackInfo];
}

- (void)nudgeVolume:(BOOL)increase
{
    float newVolume = [_volumeControl floatValue];
    newVolume += increase ? 4.0f : -4.0f;
    [_volumeControl setFloatValue:newVolume];
    [self volumeSlider:self];
}

- (void)updateTrackInfo
{
    AppDelegate* pAppDelegate = (AppDelegate *)[NSApp delegate];
    GameMusicPlayer* pAI = [pAppDelegate musicPlayer];
    
    if ( pAI && pAI->GetMusicAlbum() )
    {
        shared_ptr< Music_Album > pAlbum = pAI->GetMusicAlbum();
        
        const Music_Album::info_t& info = pAlbum->info();
        
        [_playerWindow setTitle:[NSString stringWithFormat:@"%s - %s", info.game, info.system ]];
        
        const char* const pszAuthorName = *info.author ? info.author : "unknown";
        [_authorInfoLabel setStringValue:[NSString stringWithFormat:@"By %s", pszAuthorName]];
        
        const char* const pszCopyright = *info.copyright ? info.copyright : "unknown";
        [_copyrightInfoLabel setStringValue:[NSString stringWithFormat:@"(C) %s", pszCopyright]];
    
        const char* pszPlayText = NULL;
        if ( pAI->Playing() )
        {
            //TODO: shuffle
            int track = pAI->current().track + 1;
            int track_count = pAI->GetMusicAlbum()->track_count();
            const char* const pszTrackName = *info.song ? info.song : "untitled track";
            [_trackInfoLabel setStringValue:[NSString stringWithFormat:@"%d / %d: %s", track, track_count, pszTrackName]];
            pszPlayText = "||";
        }
        else
        {
            pszPlayText = " > ";
        }
        [_playButton setTitle:[NSString stringWithFormat:@"%s", pszPlayText]];
    }
    else
    {
        [_playerWindow setTitle:[NSString stringWithUTF8String:"Open music file to begin playback."]];
        [_authorInfoLabel setStringValue:[NSString stringWithUTF8String:""]];
        [_copyrightInfoLabel setStringValue:[NSString stringWithUTF8String:""]];
        [_trackInfoLabel setStringValue:[NSString stringWithUTF8String:"- / -: no track"]];
    }
    
    [[NSNotificationCenter defaultCenter] postNotificationName:@"TrackChanged" object:self userInfo:nil];
}

- (void)stopAndClear
{
    AppDelegate* pAppDelegate = (AppDelegate *)[NSApp delegate];
    GameMusicPlayer* pAI = [pAppDelegate musicPlayer];
    pAI->stop( true );
}

- (IBAction)playTrack:(id)sender
{
    AppDelegate* pAppDelegate = (AppDelegate *)[NSApp delegate];
    GameMusicPlayer* pAI = [pAppDelegate musicPlayer];
    pAI->toggle_pause();
}

- (IBAction)nextTrack:(id)sender
{
    AppDelegate* pAppDelegate = (AppDelegate *)[NSApp delegate];
    GameMusicPlayer* pAI = [pAppDelegate musicPlayer];
    pAI->next_track();
}

- (IBAction)previousTrack:(id)sender
{
    AppDelegate* pAppDelegate = (AppDelegate *)[NSApp delegate];
    GameMusicPlayer* pAI = [pAppDelegate musicPlayer];
    pAI->prev_track();
}

- (IBAction)favoriteTrack:(id)sender
{
    AppDelegate* pAppDelegate = (AppDelegate *)[NSApp delegate];
    [pAppDelegate favoriteCurrentTrack];
}

- (IBAction)volumeSlider:(id)sender
{
    float sliderValue = [_volumeControl floatValue];
    
    AppDelegate* pAppDelegate = (AppDelegate *)[NSApp delegate];
    GameMusicPlayer* pAI = [pAppDelegate musicPlayer];
    pAI->SetVolume( sliderValue / 100.0f );
}

@end

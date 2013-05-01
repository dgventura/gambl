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
        const float framerate = 1;
        const float frequency = 1.0f/framerate;
        _PlaybackTimer = [NSTimer scheduledTimerWithTimeInterval:frequency target:self selector:@selector(updatePlaybackUI) userInfo:nil repeats:YES];
    }
    
    return self;
}

- (void)updatePlaybackUI
{
    AppDelegate* pAppDelegate = (AppDelegate *)[NSApp delegate];
    AudioPlayer* pAI = [pAppDelegate AudioInterface];
    
    string strTemp;
    pAI->update_time( strTemp );
    NSString* pStrTemp = [[NSString alloc] initWithUTF8String:strTemp.c_str() ];
    [_playbackTimeLabel setStringValue:pStrTemp];
    
    // TODO: move to dirty update
    [self updateTrackInfo];
}

- (void)updateTrackInfo
{
    AppDelegate* pAppDelegate = (AppDelegate *)[NSApp delegate];
    AudioPlayer* pAI = [pAppDelegate AudioInterface];
    
    const Music_Album::info_t& info = pAI->GetMusicAlbum()->info();
    
    [_playerWindow setTitle:[NSString stringWithFormat:@"%s - %s", info.game, info.system ]];

    const char* const pszAuthorName = *info.author ? info.author : "unknown";
    [_authorInfoLabel setStringValue:[NSString stringWithFormat:@"By %s", pszAuthorName]];

    const char* const pszCopyright = *info.copyright ? info.copyright : "unknown";
    [_copyrightInfoLabel setStringValue:[NSString stringWithFormat:@"(C) %s", pszCopyright]];

    //TODO: shuffle
    int track = pAI->current().track + 1;
    int track_count = pAI->GetMusicAlbum()->track_count();
    const char* const pszTrackName = *info.song ? info.song : "untitled track";
    [_trackInfoLabel setStringValue:[NSString stringWithFormat:@"%d / %d: %s", track, track_count, pszTrackName]];
}

- (IBAction)playTrack:(id)sender
{
    AppDelegate* pAppDelegate = (AppDelegate *)[NSApp delegate];
    AudioPlayer* pAI = [pAppDelegate AudioInterface];
    pAI->play_current();
}

- (IBAction)nextTrack:(id)sender
{
    AppDelegate* pAppDelegate = (AppDelegate *)[NSApp delegate];
    AudioPlayer* pAI = [pAppDelegate AudioInterface];
    pAI->next_track();
}

- (IBAction)previousTrack:(id)sender
{
    AppDelegate* pAppDelegate = (AppDelegate *)[NSApp delegate];
    AudioPlayer* pAI = [pAppDelegate AudioInterface];
    pAI->prev_track();
}

- (IBAction)volumeSlider:(id)sender
{
    int sliderValue = [_volumeControl intValue];
    
    NSLog( @"Slide moved! Value: %d", sliderValue );
}

@end

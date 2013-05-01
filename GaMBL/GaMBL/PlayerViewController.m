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

@interface PlayerViewController ()

@end

@implementation PlayerViewController

- (id)initWithNibName:(NSString *)nibNameOrNil bundle:(NSBundle *)nibBundleOrNil
{
    self = [super initWithNibName:nibNameOrNil bundle:nibBundleOrNil];
    if (self) {

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

@end

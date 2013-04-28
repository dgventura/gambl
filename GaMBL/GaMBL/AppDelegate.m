//
//  AppDelegate.m
//  GaMBL
//
//  Created by David Ventura on 4/3/13.
//  Copyright (c) 2013 David Ventura. All rights reserved.
//

#import "AppDelegate.h"

@implementation AppDelegate

- (void)applicationDidFinishLaunching:(NSNotification *)aNotification
{
    // Insert code here to initialize your application
    
    _AudioInterface = new AudioPlayer;
    
    // try opening an .SPC file
    NSFileHandle *file = [NSFileHandle fileHandleForReadingAtPath: @"/Users/david/dev/code/GMB/GaMBL/test.nsf"];
    if (file == nil)
        NSLog(@"Failed to open file");
    else
    {
//        NSData *databuffer = [file readDataToEndOfFile];
        
//        if ( databuffer == nil )
 //           NSLog(@"Failed to read file");
 //       else
        {
            _AudioInterface->LoadFile( file );
            
            // start a timer to keep track of the playback status
            const float framerate = 1;
            const float frequency = 1.0f/framerate;
            _PlaybackTimer = [NSTimer scheduledTimerWithTimeInterval:frequency target:self selector:@selector(updatePlaybackUI) userInfo:nil repeats:YES];
        }
    }
    
    [file closeFile];
}

- (void)dealloc
{
    delete _AudioInterface;
}

- (IBAction)playTrack:(id)sender
{
    //[TrackTimeLabel setStringValue:"Played!"];
    
    _AudioInterface->play_current();
    

    
    NSLog(@"Pressed play button!");
}

- (IBAction)nextTrack:(id)sender
{
    _AudioInterface->next_track();
    [self updatePlaybackUI];
}

- (IBAction)previousTrack:(id)sender
{
    _AudioInterface->prev_track();
    [self updatePlaybackUI];
}

- (void)updatePlaybackUI
{
    _AudioInterface->update_time();
}

@end

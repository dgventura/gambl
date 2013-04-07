//
//  AppDelegate.m
//  GaMBL
//
//  Created by David Ventura on 4/3/13.
//  Copyright (c) 2013 David Ventura. All rights reserved.
//

#import "AppDelegate.h"
#include "AudioPlayer.h"

@implementation AppDelegate

- (void)applicationDidFinishLaunching:(NSNotification *)aNotification
{
    // Insert code here to initialize your application
    
    // try opening an .SPC file
    NSFileHandle *file = [NSFileHandle fileHandleForReadingAtPath: @"/Users/david/dev/code/GMB/GaMBL/test.spc"];
    if (file == nil)
        NSLog(@"Failed to open file");
    else
    {
        //[file seekToFileOffset: 10];
    
        NSData *databuffer = [file readDataToEndOfFile];
        
        if ( databuffer == nil )
            NSLog(@"Failed to read file");
        else
        {
            AudioPlayer::foo();
  //          MusicPlayer *MyPlayer = [MusicPlayer new];
  //          [MyPlayer loadFile:databuffer];
        }
    }
    
    [file closeFile];
}

- (IBAction)playTrack:(id)sender
{
    //[TrackTimeLabel setStringValue:"Played!"];
    
    NSLog(@"Pressed play button!");
}

- (IBAction)nextTrack:(id)sender
{
}

- (IBAction)previousTrack:(id)sender
{
}

@end

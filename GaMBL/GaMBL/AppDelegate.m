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
    NSFileHandle *file = [NSFileHandle fileHandleForReadingAtPath: @"/Users/david/dev/code/GMB/GaMBL/test.spc"];
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
    
    _AudioInterface->Play();
    
    NSLog(@"Pressed play button!");
}

- (IBAction)nextTrack:(id)sender
{
}

- (IBAction)previousTrack:(id)sender
{
}

@end

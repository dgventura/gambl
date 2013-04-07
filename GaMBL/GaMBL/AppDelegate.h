//
//  AppDelegate.h
//  GaMBL
//
//  Created by David Ventura on 4/3/13.
//  Copyright (c) 2013 David Ventura. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#include "AudioPlayer.h"

@interface AppDelegate : NSObject <NSApplicationDelegate>
- (IBAction)playTrack:(id)sender;
- (IBAction)nextTrack:(id)sender;
- (IBAction)previousTrack:(id)sender;

//@private MusicPlayer* _MusicPlayer;

@property (assign) IBOutlet NSWindow *window;

@end

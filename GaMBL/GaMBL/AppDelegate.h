//
//  AppDelegate.h
//  GaMBL
//
//  Created by David Ventura on 4/3/13.
//  Copyright (c) 2013 David Ventura. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#import "AudioPlayer.h"

@interface AppDelegate : NSObject <NSApplicationDelegate>
- (IBAction)playTrack:(id)sender;
- (IBAction)nextTrack:(id)sender;
- (IBAction)previousTrack:(id)sender;

@property AudioPlayer* AudioInterface;

@property (assign) IBOutlet NSWindow *window;

@end

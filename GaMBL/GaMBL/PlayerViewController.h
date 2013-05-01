//
//  PlayerViewController.h
//  GaMBL
//
//  Created by David Ventura on 4/28/13.
//  Copyright (c) 2013 David Ventura. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#import "PlayerWindow.h"

@interface PlayerViewController : NSViewController

@property IBOutlet PlayerWindow *window;
@property NSTimer* PlaybackTimer;
@property (weak) IBOutlet NSTextField *playbackTimeLabel;

- (void)updatePlaybackUI;
- (IBAction)playTrack:(id)sender;
- (IBAction)previousTrack:(id)sender;
- (IBAction)nextTrack:(id)sender;

@end

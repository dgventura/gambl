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
@property (weak) IBOutlet NSTextField *trackInfoLabel;
@property (weak) IBOutlet NSTextField *authorInfoLabel;
@property (weak) IBOutlet NSTextField *copyrightInfoLabel;
@property (strong) IBOutlet NSWindow *playerWindow;
@property (weak) IBOutlet NSSlider *volumeControl;
@property (weak) IBOutlet NSButton *previousButton;
@property (weak) IBOutlet NSButton *nextButton;

- (void)updatePlaybackUI;
- (IBAction)playTrack:(id)sender;
- (IBAction)previousTrack:(id)sender;
- (IBAction)nextTrack:(id)sender;
- (IBAction)volumeSlider:(id)sender;

@end

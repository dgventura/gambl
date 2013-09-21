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

@property NSTimer* PlaybackTimer;

@property (strong) IBOutlet NSWindow *playerWindow;
@property (weak) IBOutlet NSTextField *playbackTimeLabel;
@property (weak) IBOutlet NSTextField *trackInfoLabel;
@property (weak) IBOutlet NSTextField *authorInfoLabel;
@property (weak) IBOutlet NSTextField *copyrightInfoLabel;
@property (weak) IBOutlet NSSlider *volumeControl;
@property (weak) IBOutlet NSButton *previousButton;
@property (weak) IBOutlet NSButton *nextButton;
@property (weak) IBOutlet NSButton *playButton;

- (void)updatePlaybackUI;
- (void)nudgeVolume:(BOOL)increase;
- (void)stopAndClear;

- (IBAction)playTrack:(id)sender;
- (IBAction)previousTrack:(id)sender;
- (IBAction)nextTrack:(id)sender;
- (IBAction)favoriteTrack:(id)sender;
- (IBAction)volumeSlider:(id)sender;

@end

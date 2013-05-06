//
//  AppDelegate.h
//  GaMBL
//
//  Created by David Ventura on 4/3/13.
//  Copyright (c) 2013 David Ventura. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#import "AudioPlayer.h"

@interface AppDelegate : NSObject <NSApplicationDelegate, NSOpenSavePanelDelegate>

@property AudioPlayer* AudioInterface;
@property (weak) IBOutlet NSMenuItem *shortMenuItem;
@property (weak) IBOutlet NSMenuItem *normalMenuItem;
@property (weak) IBOutlet NSMenuItem *extendedMenuItem;
@property (weak) IBOutlet NSMenuItem *endlessMenuItem;
@property (weak) IBOutlet NSMenuItem *playbackMenu;
@property (unsafe_unretained) IBOutlet NSPanel *soundPanel;

- (IBAction)open:(id)sender;
- (IBAction)playbackMenu:(id)sender;
- (IBAction)viewMenu:(id)sender;

- (BOOL)application:(NSApplication *)theApplication openFile:(NSString *)filename;
- (BOOL)panel:(id)sender shouldShowFilename:(NSString *)filename;
- (BOOL)validateMenuItem:(NSMenuItem *)item;

@end

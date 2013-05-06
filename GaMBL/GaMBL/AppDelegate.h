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

- (IBAction)open:(id)sender;
- (IBAction)playbackMenu:(id)sender;

- (BOOL)application:(NSApplication *)theApplication openFile:(NSString *)filename;
- (BOOL)panel:(id)sender shouldShowFilename:(NSString *)filename;


@end

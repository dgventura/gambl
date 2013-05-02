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
- (BOOL)panel:(id)sender shouldShowFilename:(NSString *)filename;

@end

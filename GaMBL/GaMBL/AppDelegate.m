//
//  AppDelegate.m
//  GaMBL
//
//  Created by David Ventura on 4/3/13.
//  Copyright (c) 2013 David Ventura. All rights reserved.
//

#import "AppDelegate.h"
#import <Foundation/NSString.h>

@implementation AppDelegate

- (void)applicationDidFinishLaunching:(NSNotification *)aNotification
{
    _AudioInterface = new AudioPlayer;
    
    // try opening an .SPC file
    NSFileHandle *file = [NSFileHandle fileHandleForReadingAtPath: @"/Users/david/dev/code/GMB/GaMBL/test.spc"];
    if (file == nil)
        NSLog(@"Failed to open file");
    else
    {
        _AudioInterface->LoadFile( file );
    }
    
    [file closeFile];
}

- (void)dealloc
{
    delete _AudioInterface;
}

@end

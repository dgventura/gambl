//
//  PlayerWindow.m
//  GaMBL
//
//  Created by David Ventura on 4/28/13.
//  Copyright (c) 2013 David Ventura. All rights reserved.
//

#import "PlayerWindow.h"
#import "PlayerViewController.h"

@implementation PlayerWindow

- (id)initWithFrame:(NSRect)frame
{
    self = [super initWithFrame:frame];
    if (self) {
        // Initialization code here.
    }
    
    return self;
}

- (BOOL)acceptsFirstResponder
{
    return YES;
}

- (void)drawRect:(NSRect)dirtyRect
{
    // Drawing code here.
}

- (void)keyDown:(NSEvent *)theEvent
{
    const int keyCode = [theEvent keyCode];
    switch(keyCode)
    {
        case 49: // space
            [_viewController playTrack:self];
            break;
        case 123: // left
            [_viewController previousTrack:self];
            break;
        case 124: // right
            [_viewController nextTrack:self];
            break;
        case 27: // -
        case 24: // +
            [_viewController nudgeVolume:(keyCode == 24)];
            break;
        case 53: // esc
            [_viewController stopAndClear];
            break;
        default:
            [super keyDown:theEvent];
    }
}

@end

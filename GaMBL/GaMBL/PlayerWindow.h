//
//  PlayerWindow.h
//  GaMBL
//
//  Created by David Ventura on 4/28/13.
//  Copyright (c) 2013 David Ventura. All rights reserved.
//

#import <Cocoa/Cocoa.h>

@class PlayerViewController;

@interface PlayerWindow : NSView

@property (strong) IBOutlet PlayerViewController *viewController;

- (void)keyDown:(NSEvent *)theEvent;
- (BOOL)acceptsFirstResponder;

@end

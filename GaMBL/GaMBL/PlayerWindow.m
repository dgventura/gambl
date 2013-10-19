//
//  PlayerWindow.m
//  GaMBL
//
//  Created by David Ventura on 4/28/13.
//  Copyright (c) 2013 David Ventura. All rights reserved.
//

#import "PlayerWindow.h"
#import "PlayerViewController.h"
#import "AppDelegate.h"

@implementation PlayerWindow

- (void)awakeFromNib
{
    NSLog(@"[%@ %@]", NSStringFromClass([self class]), NSStringFromSelector(_cmd));
    [self registerForDraggedTypes:[NSArray arrayWithObjects:NSFilenamesPboardType, nil]];
}

- (NSDragOperation)draggingEntered:(id <NSDraggingInfo>)sender
{
    NSPasteboard *pboard;
    NSDragOperation sourceDragMask;
    
    sourceDragMask = [sender draggingSourceOperationMask];
    pboard = [sender draggingPasteboard];
    
    if ( [[pboard types] containsObject:NSFilenamesPboardType] ) {
        if (sourceDragMask & NSDragOperationLink) {
            return NSDragOperationLink;
        } else if (sourceDragMask & NSDragOperationCopy) {
            return NSDragOperationCopy;
        }
    }
    return NSDragOperationNone;
}

- (BOOL)performDragOperation:(id <NSDraggingInfo>)sender
{
    NSPasteboard *pboard;
    NSDragOperation sourceDragMask;
    
    sourceDragMask = [sender draggingSourceOperationMask];
    pboard = [sender draggingPasteboard];
    
    if ( [[pboard types] containsObject:NSFilenamesPboardType] ) {
        NSArray *files = [pboard propertyListForType:NSFilenamesPboardType];
        
        //TODO: maybe support special usage (like adding to favorites??) if modifier keys are used
        // Depending on the dragging source and modifier keys,
        // the file data may be copied or linked
        AppDelegate* pAppDelegate = (AppDelegate *)[NSApp delegate];
        /*if (sourceDragMask & NSDragOperationLink) {
            [pAppDelegate application:[NSApplication sharedApplication] openFile:files[0]];
        } else {
            [pAppDelegate application:[NSApplication sharedApplication] openFile:files[0]];
        }*/
        [pAppDelegate enqueueMultipleFiles:files :YES];
    }
    return YES;
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

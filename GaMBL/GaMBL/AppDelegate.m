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
 
#if 0
    // try opening an .SPC file
    NSFileHandle *file = [NSFileHandle fileHandleForReadingAtPath: @"/Users/david/dev/code/GMB/GaMBL/test.spc"];
    if (file == nil)
        NSLog(@"Failed to open file");
    else
    {
        _AudioInterface->LoadFile( file );
    }
    
    [file closeFile];
#endif
}

- (IBAction)open:(id)sender
{
    // Create the File Open Dialog class.
    NSOpenPanel* openDlg = [NSOpenPanel openPanel];
    
    // Enable the selection of files in the dialog.
    [openDlg setCanChooseFiles:YES];
    
    // Enable the selection of directories in the dialog.
    [openDlg setAllowsMultipleSelection:NO];
    
    // Display the dialog.  If the OK button was pressed,
    // process the files.
    //TODO: file file types
    //TODO: ignore bad file types
    //TODO: default beahvior is enqueue now but with no feed back?!?!
    if ( [openDlg runModalForDirectory:nil file:nil] == NSOKButton )
    {
        // Get an array containing the full filenames of all
        // files and directories selected.
        NSArray* files = [openDlg filenames];
        
        // Loop through all the files and process them.
        for( int i = 0; i < [files count]; i++ )
        {
            NSString* fileName = [files objectAtIndex:i];
            
            NSFileHandle *file = [NSFileHandle fileHandleForReadingAtPath:fileName];
            if (file == nil)
                NSLog(@"Failed to open file");
            else
            {
                _AudioInterface->LoadFile( file );
            }
            
            [file closeFile];
        }
    }
}

- (void)dealloc
{
    delete _AudioInterface;
}

@end

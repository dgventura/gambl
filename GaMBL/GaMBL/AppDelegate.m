//
//  AppDelegate.m
//  GaMBL
//
//  Created by David Ventura on 4/3/13.
//  Copyright (c) 2013 David Ventura. All rights reserved.
//

#import "AppDelegate.h"
#import <Foundation/NSString.h>
#import <AppKit/NSWorkspace.h>

#define GAMBL_AUTOLOADTEST 0

@implementation AppDelegate

- (void)applicationDidFinishLaunching:(NSNotification *)aNotification
{
    _AudioInterface = new AudioPlayer;
 
#if GAMBL_AUTOLOADTEST
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

-(BOOL)panel:(id)sender shouldShowFilename:(NSString *)filename
{
    NSString* ext = [filename pathExtension];
    if (ext == @"" || ext == @"/" || ext == nil || ext == NULL || [ext length] < 1) {
        return TRUE;
    }
    
    //NSLog(@"Ext: '%@'", ext);
    
    NSEnumerator* tagEnumerator = [[NSArray arrayWithObjects:@"spc", @"nsf", @"gbs", @"gym", @"rar", @"zip", nil] objectEnumerator];
    NSString* allowedExt;
    while ((allowedExt = [tagEnumerator nextObject]))
    {
        if ([ext caseInsensitiveCompare:allowedExt] == NSOrderedSame)
        {
            return TRUE;
        }
    }
    
    return FALSE;
}

- (IBAction)open:(id)sender
{
    // Create the File Open Dialog class.
    NSOpenPanel* openDlg = [NSOpenPanel openPanel];
    
    [openDlg setCanChooseFiles:YES];
    [openDlg setAllowsMultipleSelection:NO];
    [openDlg setDirectoryURL:[NSURL URLWithString:NSHomeDirectory()]];
    [openDlg setDelegate:self];
    
    // Display the dialog.  If the OK button was pressed,
    // process the files.
    //TODO: default beahvior is enqueue now but with no feed back?!?!
    if ( [openDlg runModal] == NSFileHandlingPanelOKButton )
    {
        // Get an array containing the full filenames of all
        // files and directories selected.
        NSArray* files = [openDlg filenames];
        
        // Loop through all the files and process them.
        for( int i = 0; i < [files count]; i++ )
        {
            NSString* fileName = [files objectAtIndex:i];
            
            [[NSApp delegate] application:[NSApplication sharedApplication] openFile:fileName];
        }
    }
}

- (BOOL)application:(NSApplication *)theApplication openFile:(NSString *)filename
{
    BOOL bFileOk = NO;
    
    NSFileHandle *file = [NSFileHandle fileHandleForReadingAtPath:filename];
    if (file == nil)
        NSLog(@"Failed to open file");
    else
    {
        bFileOk = _AudioInterface->LoadFile( file );
        
        if ( bFileOk )
        {
            [[NSDocumentController sharedDocumentController] noteNewRecentDocumentURL:[NSURL fileURLWithPath:filename]];
        }
    }
    [file closeFile];
    
    return bFileOk;
}

- (void)dealloc
{
    delete _AudioInterface;
}

@end

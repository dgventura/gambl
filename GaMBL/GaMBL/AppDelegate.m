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
#include "favorites.h"

#define GAMBL_AUTOLOADTEST 0

//TODO: is there a way to share tags with interface builder to prevent these getting out of sync???
const int GCTRL_EXPORT          = 104;

const int GCTRL_SHOWSOUNDPANEL  = 211;
const int GCTRL_SHUFFLE         = 301;
const int GCTRL_SKIPSHORT       = 302;
const int GCTRL_EXTENDCUR       = 303;
const int GCTRL_LENSHORT        = 311;
const int GCTRL_LENNORM         = 312;
const int GCTRL_LENEXT          = 313;
const int GCTRL_LENDENDLESS     = 314;

const int GCTRL_MENU_FAVORITES  = 400;
const int GCTRL_ADDFAVORITE     = 401;
const int GCTRL_PLAYFAVORITES   = 402;
const int GCTRL_RESETFAVORITES  = 403;


@implementation AppDelegate

- (void)applicationDidFinishLaunching:(NSNotification *)aNotification
{
    _musicPlayer = new GameMusicPlayer;
 
#if GAMBL_AUTOLOADTEST
    // try opening an .SPC file
    NSFileHandle *file = [NSFileHandle fileHandleForReadingAtPath: @"/Users/david/dev/code/GMB/GaMBL/test.spc"];
    if (file == nil)
        NSLog(@"Failed to open file");
    else
    {
        _musicPlayer->LoadFile( file );
    }
    
    [file closeFile];
#endif
    
    // populate favorites menu
    [self addFavoritesToMenu];
}

-(BOOL)panel:(id)sender shouldShowFilename:(NSString *)filename
{
    NSString* ext = [filename pathExtension];
    if ( ext == nil || ext == NULL || [ext length] < 1 || [ext compare:@"/"] == NSOrderedSame )
    {
        return TRUE;
    }
    
    NSDictionary *info = [[NSBundle mainBundle] infoDictionary];
    NSArray *documentTypes = [info objectForKey:@"CFBundleDocumentTypes"];
    NSMutableArray *fileTypes = [[NSMutableArray alloc] init];
    NSAssert( documentTypes && info, @"" );
    for ( NSDictionary *item in documentTypes )
    {
        NSArray *tempTypes = [item objectForKey:@"CFBundleTypeExtensions"];
        [fileTypes addObjectsFromArray:tempTypes];
    }
    NSAssert( fileTypes.count, @"No filetypes found in bundle to support." );
    
    NSEnumerator* tagEnumerator = [fileTypes objectEnumerator];
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
    [openDlg setAllowsMultipleSelection:YES];
    [openDlg setDirectoryURL:[NSURL URLWithString:NSHomeDirectory()]];
    [openDlg setDelegate:self];
    
    // Display the dialog.  If the OK button was pressed,
    // process the files.
    //TODO: default beahvior is enqueue now but with no feedback?!?!
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

-(IBAction)fileMenu:(id)sender
{
    const int nControlTag = [sender tag];
    switch ( nControlTag )
    {
        case GCTRL_EXPORT:
            _musicPlayer->RecordCurrentTrack( false );
            break;
    }
}

- (IBAction)playbackMenu:(id)sender
{
    const int nControlTag = [sender tag];
    NSMenuItem *menuItem = (NSMenuItem *)sender;
    switch ( nControlTag )
    {
        case GCTRL_SHUFFLE:
            [menuItem setState:![menuItem state]];
            _musicPlayer->SetShuffle( [menuItem state] == NSOnState );
            break;
        case GCTRL_SKIPSHORT:
            [menuItem setState:![menuItem state]];
            _musicPlayer->SetSkipShortTracks( [menuItem state] == NSOnState );
            break;
        case GCTRL_EXTENDCUR:
            _musicPlayer->ExtendCurrent();
            break;
        case GCTRL_LENSHORT:
        case GCTRL_LENNORM:
        case GCTRL_LENEXT:
        case GCTRL_LENDENDLESS:
            _musicPlayer->SetPlayLength( nControlTag - GCTRL_LENSHORT );
            [_shortMenuItem setState:(nControlTag == GCTRL_LENSHORT ? NSOnState : NSOffState)];
            [_normalMenuItem setState:(nControlTag == GCTRL_LENNORM ? NSOnState : NSOffState)];
            [_extendedMenuItem setState:(nControlTag == GCTRL_LENEXT ? NSOnState : NSOffState)];
            [_endlessMenuItem setState:(nControlTag == GCTRL_LENDENDLESS ? NSOnState : NSOffState)];
            break;
        default:
            assert(0);
            break;
    }
}

- (IBAction)viewMenu:(id)sender
{
    const int nControlTag = [sender tag];
    NSMenuItem *menuItem = (NSMenuItem *)sender;
    switch ( nControlTag )
    {
        case GCTRL_SHOWSOUNDPANEL:
            [menuItem setState:![menuItem state]];
            [_soundPanel setIsVisible:[menuItem state] == NSOnState];
            break;
        default:
            assert(0);
            break;
    }
}

- (IBAction)favoritesMenu:(id)sender
{
    const int nControlTag = [sender tag];
    //NSMenuItem *menuItem = (NSMenuItem *)sender;
    //NSString *favoriteText = [menuItem title];
    switch ( nControlTag )
    {
        case GCTRL_ADDFAVORITE:
            [self favoriteCurrentTrack];
            //[self addFavoritesToMenu];
            break;
        case GCTRL_PLAYFAVORITES:
            [self playFavorites];
            break;
        case GCTRL_RESETFAVORITES:
            [self resetFavorites];
            [self addFavoritesToMenu];
            break;
        default:
            //assert(0);
            break;
    }
}

- (void)favoriteCurrentTrack
{
    if ( !_musicPlayer->CurrentTrackOk() )
        return;
    
    add_favorite( _musicPlayer->GetCurrentTrack(), _musicPlayer->GetMusicAlbum() );
    [self addFavoritesToMenu];
}

- (void)recordTrackPushed
{
    _musicPlayer->RecordCurrentTrack( false );
}

- (NSMutableArray *)getFavoritePaths
{
    NSMutableArray *favorites = [[NSMutableArray alloc] init];
    
    char szPath[PATH_MAX];
    const FSRef& dir = favorites_dir();
    OSStatus err = FSRefMakePath( &dir, (UInt8*)szPath, PATH_MAX );
    
    NSURL *directoryURL = [NSURL fileURLWithPath:[NSString stringWithUTF8String:szPath] isDirectory:YES];
    assert(directoryURL);
    
    NSFileManager *fileManager = [NSFileManager defaultManager];
    NSArray *keys = [NSArray arrayWithObject:NSURLIsDirectoryKey];
    
    NSDirectoryEnumerator *enumerator = [fileManager
                                         enumeratorAtURL:directoryURL
                                         includingPropertiesForKeys:keys
                                         options:0
                                         errorHandler:^(NSURL *url, NSError *error) {
                                             // Handle the error.
                                             // Return YES if the enumeration should continue after the error.
                                             return YES;
                                         }];
    
    for ( NSURL *url in enumerator )
    {
        NSError *error;
        NSNumber *isDirectory = nil;
        if ( ![url getResourceValue:&isDirectory forKey:NSURLIsDirectoryKey error:&error] )
        {
            // handle error
        }
        else if ( ![isDirectory boolValue] )
        {
            
            FSRef targetReference;
            Boolean wasAliased, isFolder;
            err = FSPathMakeRef( (UInt8*)[[url path] UTF8String], &targetReference, NULL );
            err = FSResolveAliasFile( &targetReference, TRUE, &isFolder, &wasAliased );
            err = FSRefMakePath( &targetReference, (UInt8*)szPath, sizeof(szPath) );
 
            [favorites addObject:[NSString stringWithUTF8String:szPath]];
        }
    }
    
    return favorites;
}

- (void)playFavorites
{
    // clear all current playback status
    _musicPlayer->Stop( true );

    NSMutableArray *favorites = [self getFavoritePaths];
    
    for ( id path in favorites )
    {
        NSLog( @"Queueing favorite: %@", path );
        NSFileHandle *file = [NSFileHandle fileHandleForReadingAtPath:path];
        _musicPlayer->LoadFile( file );
    }
}

- (void)resetFavorites
{
    FSRef dir = favorites_dir();
    char szPath[PATH_MAX];
    FSRefMakePath( &dir, (UInt8*)szPath, sizeof(szPath) );
    
    NSFileManager *fileManager = [NSFileManager defaultManager];
    NSDirectoryEnumerator *en = [fileManager enumeratorAtPath:[NSString stringWithUTF8String:szPath]];
    
    NSString *path;
    NSError *myError;
    while ( path = [en nextObject] )
    {
        BOOL res = [fileManager removeItemAtPath:[NSString stringWithFormat:@"%s/%@", szPath, path] error:&myError];
        if ( !res )
        {
            NSLog( @"Error removing path: %@ ", path );
        }
    }
}

- (void)addFavoritesToMenu
{
    NSMenu *targetMenu = self.favoritesMenu.submenu;
    [targetMenu removeAllItems];
    
    NSMutableArray *favorites = [self getFavoritePaths];
    for ( id path in favorites )
    {
        NSMenuItem* newEntry = [targetMenu addItemWithTitle:[path lastPathComponent] action:@selector(favoritesMenu:) keyEquivalent:@""];
        [newEntry setEnabled:YES];
        [newEntry setHidden:NO];
        NSAssert( newEntry, @"" );
    }
}

- (BOOL)validateMenuItem:(NSMenuItem *)item
{
    // disable playback menu when nothing is loaded
    if ( [item parentItem] == _playbackMenu )
        return _musicPlayer->GetMusicAlbum() ? YES : NO;
    else if ( [item tag] == GCTRL_EXPORT )
        return _musicPlayer->Playing() ? YES : NO;
    else if ( [item tag] == GCTRL_SHOWSOUNDPANEL )
    {
        [item setState:[_soundPanel isVisible]];
    }

    return YES;
}

- (BOOL)application:(NSApplication *)theApplication openFile:(NSString *)filename
{
    if ( _musicPlayer == NULL )
    {
        _musicPlayer = new GameMusicPlayer;
    }
    
    _musicPlayer->Stop( true );
    
    BOOL bFileOk  = [self enqueueSingleFile:filename];
    
    return bFileOk;
}

- (BOOL)enqueueSingleFile:(NSString *)filename
{
    BOOL bFileOk = NO;
    
    NSFileHandle *file = [NSFileHandle fileHandleForReadingAtPath:filename];
    if ( file == nil )
        NSLog( @"Failed to open file %@", filename );
    else
    {
        bFileOk = _musicPlayer->LoadFile( file );
        
        if ( bFileOk )
        {
            [[NSDocumentController sharedDocumentController] noteNewRecentDocumentURL:[NSURL fileURLWithPath:filename]];
        }
    }
    [file closeFile];
    
    return bFileOk;
}

- (void)enqueueMultipleFiles:(NSArray *)fileNames :(BOOL)bResetPlayer
{
    if ( bResetPlayer )
        _musicPlayer->Stop( true );
    
    for (NSString *currentName in fileNames)
    {
        [self enqueueSingleFile:currentName];
    }
}

- (void)dealloc
{
    delete _musicPlayer;
}

- (void)keyUp:(NSEvent*)event
{
    //TODO: to we need to handle this??
}

@end

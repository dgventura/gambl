//
//  ChannelViewController.m
//  GaMBL
//
//  Created by David Ventura on 5/5/13.
//  Copyright (c) 2013 David Ventura. All rights reserved.
//

#import "ChannelViewController.h"
#import "AppDelegate.h"

@interface ChannelViewController ()

@end

@implementation ChannelViewController

- (id)initWithNibName:(NSString *)nibNameOrNil bundle:(NSBundle *)nibBundleOrNil
{
    self = [super initWithNibName:nibNameOrNil bundle:nibBundleOrNil];
    if (self) {
        // Initialization code here.
    }
    
    return self;
}

- (void)updateSoundAttributes
{
    AppDelegate* pAppDelegate = (AppDelegate *)[NSApp delegate];
    AudioPlayer* pAI = [pAppDelegate AudioInterface];
    
    // TODO: find a way to handle this on track changed/loaded... callback??
    // probably should study what normal models are for syncing data across multiple views
    [self syncEmulatorChannelsToUI];
    
    int mask = 0;
    mask |= ((_channel1Button.state == NSOffState)) << 0;
    mask |= ((_channel2Button.state == NSOffState)) << 1;
    mask |= ((_channel3Button.state == NSOffState)) << 2;
    mask |= ((_channel4Button.state == NSOffState)) << 3;
    mask |= ((_channel5Button.state == NSOffState)) << 4;
    mask |= ((_channel6Button.state == NSOffState)) << 5;
    mask |= ((_channel7Button.state == NSOffState)) << 6;
    mask |= ((_channel8Button.state == NSOffState)) << 7;
    
    pAI->SetChannelMask( mask );
    
    BOOL bCustomSound = _enableSoundButton.state == NSOnState;
    [_trebleControl setEnabled:bCustomSound];
    [_bassControl setEnabled:bCustomSound];
    [_stereoControl setEnabled:bCustomSound];
    float fTreble = [_trebleControl floatValue] * 0.01f;
    float fBass =  [_bassControl floatValue] * 0.01f;
    float fStereo = [_stereoControl floatValue] * 0.01f;

    pAI->SetEqValues( bCustomSound, fTreble, fBass, fStereo );
}

- (void)syncEmulatorChannelsToUI
{
    if ( !self.channelButtons.count )
    {
        self.channelButtons = [[NSArray alloc] initWithObjects:_channel1Button, _channel2Button, _channel3Button, _channel4Button,
                               _channel5Button, _channel6Button, _channel7Button, _channel8Button, nil];
    }
    
    AppDelegate* pAppDelegate = (AppDelegate *)[NSApp delegate];
    AudioPlayer* pAI = [pAppDelegate AudioInterface];
    
    shared_ptr<Music_Album> pMA = pAI->GetMusicAlbum();
    NSMutableArray *channelNames = [[NSMutableArray alloc] init];
    const char** ppChannelName = pMA->info().channel_names;
    for ( int i = 0; i < pMA->info().channel_count; ++i )
    {
        [channelNames addObject:[NSString stringWithUTF8String:*ppChannelName]];
        ++ppChannelName;
    }
    [self setChannelNames:channelNames];
}

- (void)setChannelNames:(NSArray *)channelNames
{
    int counter = 0;
    for ( id button in _channelButtons )
    {
        if ( counter < channelNames.count )
        {
            [button setTitle:[channelNames objectAtIndex:counter]];
            [button setEnabled:YES];
        }
        else
        {
            [button setTitle:@"disabled"];
            [button setEnabled:NO];
        }
        ++counter;
    }
}

- (IBAction)customSoundEnable:(id)sender
{
    [self updateSoundAttributes];
}

- (IBAction)trebleSliderMoved:(id)sender
{
    [self updateSoundAttributes];
}

- (IBAction)bassSliderMoved:(id)sender
{
    [self updateSoundAttributes];
}

- (IBAction)stereoSliderMoved:(id)sender
{
    [self updateSoundAttributes];
}
@end
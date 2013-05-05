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
    
    int mask = 0;
    mask |= ((_square1Button.state == NSOffState)) << 0;
    mask |= ((_square2Button.state == NSOffState)) << 1;
    mask |= ((_triangleButton.state == NSOffState)) << 2;
    mask |= ((_noiseButton.state == NSOffState)) << 3;
    mask |= ((_dmcButton.state == NSOffState)) << 4;
    
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

- (IBAction)customSoundEnable:(id)sender
{
    [self updateSoundAttributes];
}

- (IBAction)square1Enable:(id)sender
{
    [self updateSoundAttributes];
}

- (IBAction)square2Enable:(id)sender
{
    [self updateSoundAttributes];
}

- (IBAction)triangleEnable:(id)sender
{
    [self updateSoundAttributes];
}

- (IBAction)noiseEnable:(id)sender
{
    [self updateSoundAttributes];
}

- (IBAction)dmcEnable:(id)sender
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
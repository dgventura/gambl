//
//  ChannelViewController.h
//  GaMBL
//
//  Created by David Ventura on 5/5/13.
//  Copyright (c) 2013 David Ventura. All rights reserved.
//

#import <Cocoa/Cocoa.h>

@interface ChannelViewController : NSViewController

@property (weak) IBOutlet NSButton *enableSoundButton;
@property (weak) IBOutlet NSButton *channel1Button;
@property (weak) IBOutlet NSButton *channel2Button;
@property (weak) IBOutlet NSButton *channel3Button;
@property (weak) IBOutlet NSButton *channel4Button;
@property (weak) IBOutlet NSButton *channel5Button;
@property (weak) IBOutlet NSButton *channel6Button;
@property (weak) IBOutlet NSButton *channel7Button;
@property (weak) IBOutlet NSButton *channel8Button;

@property (weak) IBOutlet NSSlider *trebleControl;
@property (weak) IBOutlet NSSlider *bassControl;
@property (weak) IBOutlet NSSlider *stereoControl;

@property (nonatomic, strong) NSArray *channelButtons;

- (IBAction)customSoundEnable:(id)sender;
- (IBAction)trebleSliderMoved:(id)sender;
- (IBAction)bassSliderMoved:(id)sender;
- (IBAction)stereoSliderMoved:(id)sender;
- (IBAction)exportStems:(id)sender;

- (void)updateSoundAttributes;
- (void)setChannelNames:(NSArray *)channelNames;
@end

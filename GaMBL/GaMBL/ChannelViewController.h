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
@property (weak) IBOutlet NSButton *square1Button;
@property (weak) IBOutlet NSButton *square2Button;
@property (weak) IBOutlet NSButton *triangleButton;
@property (weak) IBOutlet NSButton *noiseButton;
@property (weak) IBOutlet NSButton *dmcButton;
@property (weak) IBOutlet NSSlider *trebleControl;
@property (weak) IBOutlet NSSlider *bassControl;
@property (weak) IBOutlet NSSlider *stereoControl;

- (IBAction)customSoundEnable:(id)sender;
- (IBAction)square1Enable:(id)sender;
- (IBAction)square2Enable:(id)sender;
- (IBAction)triangleEnable:(id)sender;
- (IBAction)noiseEnable:(id)sender;
- (IBAction)dmcEnable:(id)sender;
- (IBAction)trebleSliderMoved:(id)sender;
- (IBAction)bassSliderMoved:(id)sender;
- (IBAction)stereoSliderMoved:(id)sender;

- (void)updateSoundAttributes;
@end

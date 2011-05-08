//
//  PWWorm.m
//  PoopWorms
//
//  Created by Mike Rotondo on 5/7/11.
//  Copyright 2011 Stanford. All rights reserved.
//

#import "PWWorm.h"
#import "EWTiming.h"
#import "PWSplotch.h"
#import "PoopWormsAppDelegate.h"
#import "PWViewController.h"

@implementation PWWorm
@synthesize notes, durationInBeats, layer, creating, splotchWorm, beatsSinceLastNote, sequence, age, lastEvent;

- (id) initWithView:(UIView*)view
{
    self = [super init];
    if (self) {
        self.notes = [NSMutableArray arrayWithCapacity:10];
        
        self.sequence = [[EWSequence new] autorelease];
        [self.sequence record];
        
        self.creating = YES;
        self.splotchWorm = [[[PWSplotchWorm alloc] initWithView:view] autorelease];
        self.splotchWorm.delegate = ((PoopWormsAppDelegate*)[UIApplication sharedApplication].delegate).viewController;
        [self.splotchWorm startWorm:CGPointMake(0, 400)];
        self.beatsSinceLastNote = 0;
        
        [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(tick) name:tickNotification object:nil];
    }
    return self;
}

- (void)dealloc
{
    [[NSNotificationCenter defaultCenter] removeObserver:self];
    
    [super dealloc];
}

- (void) addNoteWithPitch:(float)pitchPercent
{
    EWPitchEvent *event = [[[EWPitchEvent alloc] initWithPitch:pitchPercent] autorelease];
    event.worm = self;
    event.splotch = [self.splotchWorm addToWorm:CGPointMake(self.sequence.pos * 20, 400 + event.pitch * 200) tapped:YES];
    
    [self.sequence addEvent:event];
    
    self.lastEvent = event;
    
    self.beatsSinceLastNote = 0;
}

- (void) tick
{
    age++;
    
//    [self.sequence drift:1 - exp(-0.0001 * age)]; // tom: too hard!
    [self.sequence decay:1 - exp(-0.0001 * age)];
    
    if (self.creating)
    {
        if (self.beatsSinceLastNote != 0)
        {
            [self.splotchWorm addToWorm:CGPointMake((self.sequence.pos) * 20,
                                                    400 + self.lastEvent.pitch * 200) tapped:NO];
        }
        self.beatsSinceLastNote++;
    }
    
    for( EWPitchEvent *event in [self.sequence eventsAtTick:self.sequence.pos] )
    {
        [event.splotch flash];
    }
    
    [self.splotchWorm moveWorm];
}

- (void) stopCreating
{
    self.durationInBeats = self.sequence.length;
    [self.sequence play];
    self.creating = NO;
    
    [self.splotchWorm endWorm:CGPointMake(self.durationInBeats * 20, 400)];
}

- (void) clearWorm
{
    [self.splotchWorm stopWorm];
    self.splotchWorm = nil;
}

@end

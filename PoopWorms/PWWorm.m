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
#import "AKSCSynth.h"
#import "VVOSC.h"
#import "SoundFood.h"

@interface PWWorm ()

@property (nonatomic, retain) NSNumber *activeEffectID;
@property int negativeStartOffset;

- (void)mantleSynths;
- (void)dismantleSynths;

@end

@implementation PWWorm
@synthesize notes, durationInBeats, layer, creating, splotchWorm, beatsSinceLastNote, sequence, age, lastEvent, negativeStartOffset, mating, lastDate;
// Synthesis stuffs
@synthesize groupID, busID, outputNodeID, effectInBelly;
@synthesize foodInBelly, activeEffectID, volume;

- (id) initWithView:(UIView*)view andAngle:(float)angle
{
    self = [super init];
    if (self) 
    {
        self.notes = [NSMutableArray arrayWithCapacity:10];
        
        self.sequence = [[EWSequence new] autorelease];
        [self.sequence record];
        
        self.creating = YES;
        self.splotchWorm = [[[PWSplotchWorm alloc] initWithView:view andAngle:angle andWorm:self] autorelease];
        self.splotchWorm.delegate = ((PoopWormsAppDelegate*)[UIApplication sharedApplication].delegate).viewController;
        self.negativeStartOffset = -100;
        [self.splotchWorm startWorm:CGPointMake(self.negativeStartOffset, 400)];
        self.beatsSinceLastNote = 0;
        
        [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(tick) name:tickNotification object:nil];
        
        [self mantleSynths];
    }
    return self;
}

- (void)eatEffect:(NSString *)effectName
{
    NSArray *args = [NSArray arrayWithObjects:
                     [OSCValue createWithString:@"inBus"], 
                     [OSCValue createWithInt:[self.busID intValue]],
                     nil];
    if (self.activeEffectID) 
    {
        // TODO: poop active effect
        self.activeEffectID = [[AKSCSynth sharedSynth] synthWithName:effectName 
                                                        andArguments:args 
                                                           addAction:AKReplaceAction 
                                                            targetID:self.activeEffectID];
    }
    else
    {
        self.activeEffectID = [[AKSCSynth sharedSynth] synthWithName:effectName 
                                                        andArguments:args 
                                                           addAction:AKAddBeforeAction 
                                                            targetID:self.outputNodeID];
    }
    
    // update display here
    for (PWSplotch* splotch in self.splotchWorm.wormSplotches)
    {
        if ( !splotch.active ) [splotch changeImageTo:[SoundFood imageForEffectId:self.effectInBelly] all:YES];
    }
}

- (void)mantleSynths
{
    // Synthesis stuff
    self.volume = 1;
    self.groupID = [[AKSCSynth sharedSynth] group];
    self.busID = [[AKSCSynth sharedSynth] bus];
    NSArray *outArgs = [NSArray arrayWithObjects:
                        [OSCValue createWithString:@"inBus"], 
                        [OSCValue createWithInt:[self.busID intValue]], 
                        nil];
    self.outputNodeID = [[AKSCSynth sharedSynth] synthWithName:@"OutConnector"
                                                  andArguments:outArgs
                                                     addAction:AKAddToTailAction
                                                      targetID:self.groupID];
    self.foodInBelly = 0;
    self.activeEffectID = nil;
}

- (void)dismantleSynths
{
    // TODO: call this when the worm dies
    [[AKSCSynth sharedSynth] freeAllInGroup:self.groupID];
}

- (void)dealloc
{
    [[NSNotificationCenter defaultCenter] removeObserver:self];
    
    [groupID release];
    [busID release];
    [activeEffectID release];
    
    [self.splotchWorm cleanup];
    self.splotchWorm = nil;
    for( EWPitchEvent *event in self.sequence.allEvents )
        event.worm = nil;
    self.sequence = nil;
    
    [super dealloc];
}

- (void) addNoteWithPitch:(float)pitchPercent
{
    EWPitchEvent *event = [[[EWPitchEvent alloc] initWithPitch:pitchPercent] autorelease];
    event.worm = self;
    event.splotch = [self.splotchWorm addToWorm:CGPointMake(self.negativeStartOffset + self.sequence.pos * 20, 400 + event.pitch * 200) tapped:YES];
    
    [self.sequence addEvent:event];
    
    self.lastEvent = event;
    
    self.beatsSinceLastNote = 0;
}

- (BOOL)dead
{
    return self.sequence.allEvents.count == 0;
}

- (void)tick
{
    // AGING
    if (!self.creating)
    {
        //    [self.sequence drift:1 - exp(-0.0001 * age)]; // tom: too hard!
        [self.sequence decay:1 - exp(-0.000001 * age)];
        self.volume = exp(-0.001 * age);
        
        UIView *lastSplotch = self.splotchWorm.wormSplotches.lastObject;
        BOOL offScreen = !CGRectContainsPoint(self.splotchWorm.layer.superlayer.bounds, CGPointApplyAffineTransform(lastSplotch.center, [self.splotchWorm inverseTransformForSuperview]) );
        
        BOOL dead = [self dead];
        
        if( offScreen || dead )
        {
            age += 400;
        }
        else
        {
            age++;
        }
        
        if( offScreen && dead )
        {
            [self clearWorm];
            return;
        }
        
        everySoOften++;
        if( everySoOften % 10 == 0 )
            [self.splotchWorm setAlpha:exp(-0.002 * age)];
    }
    
    if (self.creating)
    {
        if (self.beatsSinceLastNote != 0)
        {
            [self.splotchWorm addToWorm:CGPointMake(self.negativeStartOffset + self.sequence.pos * 20,
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

- (void) updateDisplay
{
    for (PWSplotch* splotch in self.splotchWorm.wormSplotches)
    {
        [splotch changeImageTo:[SoundFood imageForFoodId:self.foodInBelly] all:NO];
    }
}

- (void) stopCreating
{
    self.durationInBeats = self.sequence.length;
    [self.sequence play];
    self.creating = NO;
    
    [self.splotchWorm endWorm:CGPointMake(self.negativeStartOffset + self.durationInBeats * 20, 400)];
}

- (void)clearWorm
{
    [[NSNotificationCenter defaultCenter] removeObserver:self];
    
    [self dismantleSynths];
    
    [self.splotchWorm stopWorm];
    self.splotchWorm = nil;
}

- (CGRect)boundingBox
{
    CGRect rect = CGRectNull;
    
    for( PWSplotch *splotch in self.splotchWorm.wormSplotches )
    {
        rect = CGRectUnion( rect, CGRectMake( splotch.center.x, splotch.center.y, 1, 1 ) );
    }
    
    return rect;
}

@end

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

@interface PWWorm ()

@property (nonatomic, retain) NSMutableArray *activeDrugIDs;
@property int negativeStartOffset;

- (void)mantleSynths;
- (void)dismantleSynths;

@end

@implementation PWWorm
@synthesize notes, durationInBeats, layer, creating, splotchWorm, beatsSinceLastNote, sequence, age, lastEvent, negativeStartOffset;
// Synthesis stuffs
@synthesize groupID, busID, outputNodeID;
@synthesize foodInBelly, activeDrugIDs, volume;

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
    NSNumber *nodeID = [[AKSCSynth sharedSynth] synthWithName:effectName 
                                                 andArguments:args 
                                                    addAction:AKAddBeforeAction 
                                                     targetID:self.outputNodeID];
    [self.activeDrugIDs addObject:nodeID];
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
    self.activeDrugIDs = [NSMutableArray array];
    
    NSArray *possibleDrugs = [NSArray arrayWithObjects:
                              @"Tanh", 
                              @"CombNDelay", 
                              @"Flanger", 
                              @"PitchShift", nil];
    NSString *randomDrug = [possibleDrugs objectAtIndex:arc4random() % [possibleDrugs count]];
    NSLog(@"Spawing %@", randomDrug);
    // give SCSynth time to create groups
    [self performSelector:@selector(eatEffect:) withObject:randomDrug afterDelay:0.1];
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
    [activeDrugIDs release];
    
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
//    [self.sequence drift:1 - exp(-0.0001 * age)]; // tom: too hard!
    [self.sequence decay:1 - exp(-0.00001 * age)];
    self.volume = exp(-0.001 * age);
    
    UIView *lastSplotch = self.splotchWorm.wormSplotches.lastObject;
    BOOL offScreen = !CGRectContainsPoint(self.splotchWorm.layer.superlayer.bounds, CGPointApplyAffineTransform(lastSplotch.center, [self.splotchWorm extracted_method]) );
    
    BOOL dead = [self dead];
    
    if( offScreen || dead )
    {
        age += 40;
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
    
    [self.splotchWorm setAlpha:exp(-0.002 * age)];
    
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

@end

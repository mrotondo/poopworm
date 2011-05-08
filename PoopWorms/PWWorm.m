//
//  PWWorm.m
//  PoopWorms
//
//  Created by Mike Rotondo on 5/7/11.
//  Copyright 2011 Stanford. All rights reserved.
//

#import "PWWorm.h"
#import "PWNote.h"
#import "EWTiming.h"
#import "PWSplotch.h"

@implementation PWWorm
@synthesize notes, durationInBeats, layer, creating, splotchWorm, beatsSinceLastNote, sequence;

- (id) initWithView:(UIView*)view
{
    self = [super init];
    if (self) {
        self.notes = [NSMutableArray arrayWithCapacity:10];
        
        self.sequence = [[EWSequence new] autorelease];
        [self.sequence record];
        
        self.creating = YES;
        self.splotchWorm = [[[PWSplotchWorm alloc] initWithView:view] autorelease];
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
    int beatIndex = self.sequence.pos;
    PWNote* note = [[PWNote alloc] initWithBeatIndex:beatIndex andPitchPercent:pitchPercent];
    [self.notes addObject:note];
    PWSplotch* splotch = [self.splotchWorm addToWorm:CGPointMake(note.beatIndex * 20, 400 + note.pitchPercent * 200) tapped:YES];
    note.splotch = splotch;
    self.beatsSinceLastNote = 0;
    [self.sequence addEvent:[[EWPitchEvent alloc] initWithPitch:pitchPercent]];
}

- (void) tick
{
    if (self.creating)
    {
        PWNote* lastNote = [self.notes lastObject];
        if (lastNote.beatIndex != self.sequence.pos)
        {
            [self.splotchWorm addToWorm:CGPointMake((lastNote.beatIndex + beatsSinceLastNote) * 20,
                                                    400 + lastNote.pitchPercent * 200) tapped:NO];
        }
        self.beatsSinceLastNote++;
    }
    
    for (PWNote* note in self.notes)
    {
        if (note.beatIndex == self.sequence.pos)
        {
            [note.splotch flash];
        }
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

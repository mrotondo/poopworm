//
//  EWTiming.h
//  EWTown
//
//  Created by Tom Lieber on 5/7/11.
//  Copyright 2011 Smule. All rights reserved.
//

extern NSString *tickNotification;


@interface EWEvent : NSObject
- (void)fire;
@end

@class PWSplotch;
@class PWWorm;
@interface EWPitchEvent : EWEvent

@property (nonatomic, assign) float pitch;
@property (nonatomic, retain) PWSplotch *splotch;
@property (nonatomic, assign) PWWorm *worm;

- (id)initWithPitch:(float)pitch;

@end

@interface EWBlockEvent : EWEvent
+ (id)eventWithBlock:(void (^)(void))aBlock;
@end


@interface EWSequence : NSObject
{
    NSMutableArray *timeline; // array of sets of events, index is the beat
}

@property (nonatomic, readonly) int pos;
@property (nonatomic, assign) int length;
@property (nonatomic, readonly) NSSet *allEvents;

- (void)addEvent:(EWEvent *)event;
- (void)addEvent:(EWEvent *)event atTick:(int)tick;
- (void)poopEvent:(EWEvent *)event;
- (NSSet *)eventsAtTick:(int)tick;
- (void)drift:(float)amount; // 0-1
- (void)decay:(float)amount; // 0-1
- (void)record; // don't do this after you start playing
- (void)play;
- (void)stop;
- (void)reset;

@end


@interface EWTicker : NSObject
{
    float currentTick;
    NSDate *lastTickDate;
}

@property (nonatomic, assign) float ticksPerSecond;
@property (nonatomic, readonly) BOOL closerToPrevious; // whether we're closer to the previous tick or the next one

+ (EWTicker *)sharedTicker;

- (id)initWithBeatsPerMeasure:(int)beatsPerMeasure subdivisions:(int)subdivisions bpm:(float)bpm;
- (id)initWithTicksPerSecond:(float)theTicksPerSecond;

- (void)start;
- (void)stop;

@end

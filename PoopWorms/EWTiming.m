//
//  EWTiming.m
//  EWTown
//
//  Created by Tom Lieber on 5/7/11.
//  Copyright 2011 Smule. All rights reserved.
//

#include "EWTiming.h"

NSString *tickNotification = @"Nobody will ever see the contents of this string";

static EWTicker *g_ticker = nil;


#pragma mark -
@implementation EWEvent

- (void)fire
{
    NSLog(@"this event does nothing");
}

@end


#pragma mark -
@implementation EWPitchEvent

@synthesize pitch;

- (id)initWithPitch:(float)aPitch
{
    self = [super init];
    
    if( self )
    {
        self.pitch = aPitch;
    }
    
    return self;
}

@end


#pragma mark -
@interface EWBlockEvent ()

@property (nonatomic, retain) void (^block)(void);

- (id)initWithBlock:(void (^)(void))aBlock;

@end

@implementation EWBlockEvent

@synthesize block;

+ (id)eventWithBlock:(void (^)(void))aBlock
{
    return [[[self alloc] initWithBlock:aBlock] autorelease];
}

- (id)initWithBlock:(void (^)(void))aBlock
{
    self = [super init];
    
    if( self )
    {
        self.block = [[aBlock copy] autorelease];
    }
    
    return self;
}

- (void)dealloc
{
    self.block = nil;
    
    [super dealloc];
}

- (void)fire
{
    self.block();
}

@end


#pragma mark -
@interface EWSequence ()

@property (nonatomic, assign) BOOL recording;

@end


@implementation EWSequence

@synthesize recording;
@synthesize pos;

- (id)init
{
    self = [super init];
    
    if( self )
    {
        timeline = [NSMutableArray new];
        [timeline addObject:[NSMutableSet set]];
    }
    
    return self;
}

- (void)dealloc
{
    [self stop];
    
    [timeline release], timeline = nil;
    
    [super dealloc];
}

- (int)length
{
    return timeline.count;
}

- (void)addEvent:(EWEvent *)event
{
    // TODO: find nearest tick
    
    NSMutableSet *events = [timeline objectAtIndex:pos];
    [events addObject:event];
}

- (void)addEvent:(EWEvent *)event atTick:(int)tick
{
    if( tick >= 0 && tick < timeline.count )
    {
        NSMutableSet *events = [timeline objectAtIndex:tick];
        [events addObject:event];
    }
    else
    {
        NSLog(@"wtf is this event");
    }
}

// remove an event (i.e. poop it out)
- (void)poopEvent:(EWEvent *)event
{
    // remove it from every point in the timeline
    for( NSMutableSet *events in timeline )
    {
        [events removeObject:event];
    }
}

- (void)record
{
    recording = YES;
    
    [[NSNotificationCenter defaultCenter] removeObserver:self];
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(tick) name:tickNotification object:nil];
}

- (void)play
{
    recording = NO;
    
    [[NSNotificationCenter defaultCenter] removeObserver:self];
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(tick) name:tickNotification object:nil];
}

- (void)stop
{
    recording = NO;
    
    [[NSNotificationCenter defaultCenter] removeObserver:self];
}

- (void)reset
{
    pos = 0;
}

- (void)tick
{
    if( recording )
    {
        // grow by one
        [timeline addObject:[NSMutableSet set]];
        
        pos++;
    }
    else
    {
        pos = (pos + 1) % timeline.count;
        
        // fire all the events
        for( EWEvent *event in [timeline objectAtIndex:pos] )
            [event fire];
    }
}

@end



#pragma mark -
@interface EWTicker ()

@property (nonatomic, retain) NSTimer *timer;

- (void)_advance:(float)dt;

@end

@implementation EWTicker

- (id)initWithBeatsPerMeasure:(int)beatsPerMeasure subdivisions:(int)subdivisions bpm:(float)bpm
{
    // subdivisions = ticks / beat
    // bpm = beats / minute
    // (1 / 60) = minutes / second
    // =>
    // (ticks / beat) * (beats / minute) * (minutes / second) = (ticks / second)
    float tps = subdivisions * bpm * (1.0 / 60.0);
    
    return [self initWithTicksPerSecond:tps];
}

- (id)initWithTicksPerSecond:(float)theTicksPerSecond
{
    self = [super init];
    
    if( self )
    {
        self.ticksPerSecond = theTicksPerSecond;
    }
    
    return self;
}


#pragma mark - Properties

@synthesize ticksPerSecond;
@synthesize timer;

- (BOOL)closerToPrevious
{
    return currentTick < 0.5;
}

+ (EWTicker *)sharedTicker
{
    if( g_ticker == nil )
        g_ticker = [[EWTicker alloc] initWithBeatsPerMeasure:4 subdivisions:4 bpm:103];
    
    return g_ticker;
}

#pragma mark - Timing

- (void)start
{
    [self stop];
    self.timer = [NSTimer scheduledTimerWithTimeInterval:0.01 target:self selector:@selector(advance) userInfo:nil repeats:YES];
}

- (void)stop
{
    [self.timer invalidate];
    self.timer = nil;
}

- (void)advance
{
    if( lastTickDate == nil )
    {
        // do nothing yet
    }
    else
    {
        NSTimeInterval dt = [[NSDate date] timeIntervalSinceDate:lastTickDate];
        [self _advance:dt];
    }
    
    [lastTickDate release];
    lastTickDate = [[NSDate date] retain];
}

- (void)_advance:(float)dt
{
    currentTick += dt * ticksPerSecond;
    
    while( currentTick > 1 )
    {
        [[NSNotificationCenter defaultCenter] postNotificationName:tickNotification object:self];
        
        currentTick -= 1;
    }
}

@end
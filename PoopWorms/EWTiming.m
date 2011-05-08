//
//  EWTiming.m
//  EWTown
//
//  Created by Tom Lieber on 5/7/11.
//  Copyright 2011 Smule. All rights reserved.
//

#import "EWTiming.h"
#import "AKSCSynth.h"
#import "PWWorm.h"
#import "SoundFood.h"

NSString *tickNotification = @"Nobody will ever see the contents of this string";

static EWTicker *g_ticker = nil;


static float mtof( float midi )
{
    if( midi <= -1500 ) return (0);
    else if( midi > 1499 ) return (mtof(1499));
    else return ( pow(2,(midi-69)/12.0) * 440.0 );
}

#define LOGTWO 0.69314718055994528623
static float ftom( float freq )
{
    return (freq > 0 ? (log(freq/440.0) / LOGTWO) * 12.0 + 69 : -1500);
}


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
@synthesize splotch;
@synthesize worm;

- (id)initWithPitch:(float)aPitch
{
    self = [super init];
    
    if( self )
    {
        self.pitch = aPitch;
    }
    
    return self;
}

- (void)fire
{
    float freq = (1 - self.pitch) * 1000 + 100;
    float roundedFreq = mtof( (int)ftom( freq ) );
    
    NSArray *args = [NSArray arrayWithObjects:
                     [OSCValue createWithString:@"pitch"],
                     [OSCValue createWithInt:roundedFreq],
                     [OSCValue createWithString:@"volume"],
                     [OSCValue createWithFloat:self.worm.volume],
                     [OSCValue createWithString:@"outBus"], 
                     [OSCValue createWithInt:[self.worm.busID intValue]],
                     nil];

    [[AKSCSynth sharedSynth] synthWithName:[SoundFood synthNameForFoodId:self.worm.foodInBelly]
                              andArguments:args 
                                 addAction:AKAddToHeadAction 
                                  targetID:self.worm.groupID];
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

- (void)setLength:(int)newLength
{
    if( newLength > 0 )
    {
        if( newLength < timeline.count )
        {
            [timeline removeObjectsInRange:NSMakeRange( newLength, timeline.count - newLength )];
            if( pos >= newLength )
                pos = 0;
        }
        else if( newLength > timeline.count )
        {
            for( int i = timeline.count; i < newLength; i++ )
                [timeline addObject:[NSMutableSet set]];
        }
    }
    else
    {
        NSLog(@"wtf is this length");
    }
}

- (int)length
{
    return timeline.count;
}

- (NSSet *)allEvents
{
    NSMutableSet *all = [NSMutableSet set];
    
    for( NSSet *events in timeline )
    {
        [all unionSet:events];
    }
    
    return all;
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

- (NSSet *)eventsAtTick:(int)tick
{
    if( tick >= 0 && tick < timeline.count )
    {
        return [timeline objectAtIndex:tick];
    }
    
    return nil;
}

- (void)drift:(float)amount
{
    for( int i = 0; i < timeline.count; i++ )
    {
        NSMutableSet *events = [timeline objectAtIndex:i];
        
        for( id event in [[events copy] autorelease] )
        {
            if( (arc4random() % 1000) / 1000.0 < amount )
            {
                NSLog(@"moved one");
                [events removeObject:event];
                
                NSMutableSet *otherEvents = [timeline objectAtIndex:(i + 1) % timeline.count];
                [otherEvents addObject:event];
            }
        }
    }
}

- (void)decay:(float)amount
{
    for( NSMutableSet *events in timeline )
    {
        for( id event in [[events copy] autorelease] )
        {
            if( (arc4random() % 1000) / 1000.0 < amount )
            {
                [events removeObject:event];
            }
        }
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
        g_ticker = [[EWTicker alloc] initWithBeatsPerMeasure:4 subdivisions:16 bpm:30];
    
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

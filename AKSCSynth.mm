//
//  AKSCSynth.m
//  Artikulator
//
//  Created by Luke Iannini on 6/28/10.
//  Copyright 2010 Hello, Chair Inc. All rights reserved.
//

#import "AKSCSynth.h"
#import <AudioToolbox/AudioToolbox.h>
#include "SC_World.h"
#include "SC_HiddenWorld.h"
#include "SC_CoreAudio.h"
#include "SC_WorldOptions.h"
#include "SC_Graph.h"
#include "SC_GraphDef.h"
#include "SC_Prototypes.h"
#include "SC_Node.h"
#include "SC_DirUtils.h"
//#import "NSArray+SCSynthAdditions.h"
#import "OSCMessage+AddArguments.h"


int vpost(const char *fmt, va_list ap)
{
	char buf[512];
	vsnprintf(buf, sizeof(buf), fmt, ap);

    NSString *logString = [[NSString alloc] initWithCString:buf encoding:NSASCIIStringEncoding];
    NSLog(@"Server: %@", logString);
    [logString release];

	return 0;
}

@interface AKSCSynth ()

@property (nonatomic) WorldOptions options;
@property (nonatomic) struct World *world;
@property (nonatomic, retain) NSTimer *timer;

@property (nonatomic) NSInteger lastNodeID;
@property (nonatomic) NSInteger nextBufferNumber;

- (void)copySynthDefs;
- (void)start;
- (void)stop;
- (void)loadSounds;

@end


@implementation AKSCSynth
@synthesize OSCPort;
@synthesize synthsByName;
@synthesize soundFilesByBufferNumber;
@synthesize options, world, timer, lastNodeID, nextBufferNumber;

- (void)dealloc
{
    [synthsByName release];
    [soundFilesByBufferNumber release];
	self.OSCPort = nil;
	[super dealloc];
}

- (void)copySynthDefs
{
    NSFileManager *manager = [NSFileManager defaultManager];

    NSString *bundlePath = [[NSBundle mainBundle] bundlePath];

	NSError *error = nil;
	NSString *support = [NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES) lastObject];
    NSString *dir = [support stringByAppendingPathComponent:@"/synthdefs"];
    // Always replace synthdefs in case they change. 
    // TODO can command server to just load them from the bundle, don't need to copy them anywhere.
    if ([manager fileExistsAtPath:dir])
    {
        [manager removeItemAtPath:dir error:&error];
    }
    
    NSString *from = [bundlePath stringByAppendingPathComponent:@"/synthdefs"];
    if ([manager fileExistsAtPath:from])
    {
        [manager copyItemAtPath:from toPath:dir error:&error];
    }
}

- (void)start
{
	if (world) World_Cleanup(world);
	world = World_New(&options);
	if (!world || !World_OpenUDP(world, 57110)) return;
}

- (void)stop
{
	if (world) World_Cleanup(world);
	world = nil;
}

- (double)averageCPU
{
    return world ? world->hw->mAudioDriver->GetAvgCPU() : nil;
}

- (double)peakCPU
{
    return world ? world->hw->mAudioDriver->GetPeakCPU() : nil;
}

- (void)dumpTree
{
    OSCMessage *message = [OSCMessage createWithAddress:@"/g_dumpTree"];
    [message addInt:0]; // group ID
    [message addInt:1]; // print control values of synths
    [self.OSCPort sendThisMessage:message];
}

- (void)sendMessageInBundle:(OSCMessage *)message
{
    OSCBundle *bundle = [OSCBundle createWithElement:message];
    bundle.timeStamp = [NSDate dateWithTimeIntervalSinceNow:0.1];
    [self sendBundle:bundle];
}

- (void)sendBundle:(OSCBundle *)bundle
{
    [self.OSCPort sendThisBundle:bundle];
}

// Begin ObjC>SCServer interface

- (void)freeAll
{
    OSCMessage *message = [OSCMessage createWithAddress:@"/g_freeAll"];
    [message addInt:0];
    [self.OSCPort sendThisMessage:message];
    message = [OSCMessage createWithAddress:@"/clearSched"];
    [self.OSCPort sendThisMessage:message];
}

- (void)dumpOSC:(BOOL)flag
{
    OSCMessage *message = [OSCMessage createWithAddress:@"/dumpOSC"];
    [message addInt:flag];
    [self.OSCPort sendThisMessage:message];
}

- (NSNumber *)synthWithName:(NSString *)synthName andArguments:(NSArray *)arguments
{
    NSInteger nodeID;
    OSCMessage *s_new = [self s_newMessageWithSynth:synthName
                                       andArguments:arguments
                                             nodeID:&nodeID];
    
    //DLog(@"Sending Message: %@", s_new);
    [self sendMessageInBundle:s_new];

    return [NSNumber numberWithInteger:nodeID];
}

- (void)setNodeID:(NSInteger)nodeID
    withArguments:(NSArray *)arguments
{
    OSCMessage *n_set = [self n_setMessageWithNodeID:nodeID andArguments:arguments];

    [self sendMessageInBundle:n_set];
}

- (OSCMessage *)n_setMessageWithNodeID:(NSInteger)nodeID
                          andArguments:(NSArray *)arguments
{
    OSCMessage *message = [OSCMessage createWithAddress:@"/n_set"];

    [message addValue:[OSCValue createWithInt:nodeID]];

    [message addArguments:arguments];

    return message;
}

- (OSCMessage *)s_newMessageWithSynth:(NSString *)synthName
                         andArguments:(NSArray *)arguments
                               nodeID:(NSInteger *)nodeIDDestination
{
    OSCMessage *message = [OSCMessage createWithAddress:@"/s_new"];

    NSInteger nodeID = ++lastNodeID;
    *nodeIDDestination = nodeID;
    
    [message addString:synthName];
    [message addInt:nodeID];
    // Add action. 0=addToHead, 1=addToTail, 2=addBefore(target), 3=addAfter(target), 4=replace(and free target)
    [message addInt:0];
    // Target ID.
    [message addInt:0]; // 0 = default group.

    [message addArguments:arguments];

    return message;
}

- (OSCMessage *)b_allocReadMessageWithPath:(NSString *)path bufferNumber:(NSInteger)bufferNumber
{
    OSCMessage *message = [OSCMessage createWithAddress:@"/b_allocReadChannel"];
    [message addInt:bufferNumber];
    [message addString:path];
    [message addInt:0]; // Starting frame
    [message addInt:220500]; // Number of frames (5 seconds max for now)
    [message addInt:0]; // First channel only (granulators only support mono files)
    return message;
}

static AKSCSynth *sharedInstance = nil;

+ (void)initialize
{
    if (sharedInstance == nil)
        sharedInstance = [[self alloc] init];
}

+ (id)sharedSynth
{
    //Already set by +initialize.
    return sharedInstance;
}

+ (id)allocWithZone:(NSZone *)zone
{
    //Usually already set by +initialize.
    if (sharedInstance)
    {
        //The caller expects to receive a new object, so implicitly retain it
        //to balance out the eventual release message.
        return [sharedInstance retain];
    }
    else
    {
        //When not already set, +initialize is our caller.
        //It's creating the shared instance, let this go through.
        return [super allocWithZone: zone];
    }
}

- (void)test
{
    OSCMessage *message = [OSCMessage createWithAddress:@"/dumpOSC"];
    [message addInt:1];
    OSCBundle *bundle = [OSCBundle createWithElement:message];
    bundle.timeStamp = [NSDate dateWithTimeIntervalSinceNow:0.5];
    [self.OSCPort sendThisBundle:bundle];
}

- (void)loadSounds
{
    NSArray *sounds = [NSArray arrayWithObjects:@"a11wlk01.wav", nil];
    for (NSString *soundName in sounds)
    {
        NSString *soundPath = [[[NSBundle mainBundle] bundlePath] stringByAppendingPathComponent:soundName];
        [self.soundFilesByBufferNumber setObject:soundPath forKey:[NSNumber numberWithInt:nextBufferNumber]];
        OSCMessage *message = [self b_allocReadMessageWithPath:soundPath bufferNumber:nextBufferNumber];
#if !TARGET_IPHONE_SIMULATOR
        // crashes the simulator, maybe something with libsndfile not being loaded correctly??
        [self.OSCPort sendThisMessage:message];
#endif
        nextBufferNumber++;
    }
}

- (id)init
{
    //If sharedInstance is nil, +initialize is our caller, so initialize the instance.
    //If it is not nil, simply return the instance without re-initializing it.
    if (sharedInstance == nil)
    {
        self = [super init];
        if (self)
        {
            options = kDefaultWorldOptions;
            options.mBufLength = 1024;
            world = nil;
            lastNodeID = 1000;
            self.soundFilesByBufferNumber = [NSMutableDictionary dictionaryWithCapacity:5];

            unsigned long route = kAudioSessionOverrideAudioRoute_None;
            AudioSessionSetProperty(kAudioSessionProperty_OverrideAudioRoute, sizeof(route), &route);
            
            SetPrintFunc(vpost);
            
            [self copySynthDefs];
            
            [self start];
            
            OSCManager *manager = [[[OSCManager alloc] init] autorelease];
            self.OSCPort = [manager createNewOutputToAddress:@"127.0.0.1" atPort:57110];
            
            //[self loadSounds];
            
            //[self dumpOSC:YES];
            
            // Using a notification so we don't have to import AKSCSynth 
            // and infect AKSongViewController with .mm-ness
            [[NSNotificationCenter defaultCenter] addObserver:self 
                                                     selector:@selector(dumpTree) 
                                                         name:@"AKPostTreeNotification" 
                                                       object:nil];
            
            //[self performSelector:@selector(test) withObject:nil afterDelay:0.5];
            
            //[self testTone];
            //[self stressTest];
            //[self synthTest];
        }
    }
    return self;
}

- (void)testTone
{
    // TestTone
    OSCMessage *message = [OSCMessage createWithAddress:@"/s_new"];
    [message addString:@"FSinOscTouch"];
    [message addInt:1000];
    [message addInt:0]; // Add action (not implemented)
    [message addInt:0]; // Add target ID (not implemented)
    [message addString:@"pitch"];
    [message addInt:440];
    
    [self.OSCPort sendThisMessage:message];
}

- (void)stressTest
{
    // Stress Test (400 sines)
    OSCMessage *message = [OSCMessage createWithAddress:@"/s_new"];
    [message addString:@"SinesTest"];
    [message addInt:1001];
    [message addInt:0]; // Add action (not implemented)
    [message addInt:0]; // Add target ID (not implemented)
}

- (void)synthTest
{
    // Synth Test
    OSCMessage *message = [OSCMessage createWithAddress:@"/s_new"];
    [message addString:@"TestSynth"];
    [message addInt:1001];
    [message addInt:0]; // Add action (not implemented)
    [message addInt:0]; // Add target ID (not implemented)
    
    [self.OSCPort sendThisMessage:message];
}

@end

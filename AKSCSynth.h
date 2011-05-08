//
//  AKSCSynth.h
//  Artikulator
//
//  Created by Luke Iannini on 6/28/10.
//  Copyright 2010 Hello, Chair Inc. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "VVOSC.h"

#define AKSCSynthSyncDelay 0.5

@interface AKSCSynth : NSObject
{
    
}

@property (nonatomic, retain) OSCOutPort *OSCPort;
@property (nonatomic, copy) NSDictionary *synthsByName;
@property (nonatomic, retain) NSMutableDictionary *soundFilesByBufferNumber;

+ (id)sharedSynth;

- (double)averageCPU;
- (double)peakCPU;
- (void)dumpTree;

- (void)sendMessageInBundle:(OSCMessage *)message;
- (void)sendBundle:(OSCBundle *)bundle;

- (void)freeAll;

// Obj-SC

- (void)dumpOSC:(BOOL)flag;

- (NSNumber *)synthWithName:(NSString *)synthName andArguments:(NSArray *)arguments;
- (void)setNodeID:(NSInteger)nodeID withArguments:(NSArray *)arguments;

- (OSCMessage *)n_setMessageWithNodeID:(NSInteger)nodeID andArguments:(NSArray *)arguments;

- (OSCMessage *)s_newMessageWithSynth:(NSString *)synthName
                         andArguments:(NSArray *)arguments
                               nodeID:(NSInteger *)nodeIDDestination;

- (OSCMessage *)b_allocReadMessageWithPath:(NSString *)path 
                              bufferNumber:(NSInteger)bufferNumber;

@end

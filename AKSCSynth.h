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

typedef enum {
    AKAddToHeadAction = 0,
    AKAddToTailAction,
    AKAddBeforeAction,
    AKAddAfterAction,
    AKReplaceAction
} AKAddAction;
#define AKDefaultGroupID 0

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

- (NSNumber *)bus;
- (NSNumber *)group;
- (void)freeAllInGroup:(NSNumber *)groupNodeID;
- (NSNumber *)synthWithName:(NSString *)synthName 
               andArguments:(NSArray *)arguments;
- (NSNumber *)synthWithName:(NSString *)synthName 
               andArguments:(NSArray *)arguments 
                  addAction:(AKAddAction)addAction 
                   targetID:(NSNumber *)targetID;

- (void)setNodeID:(NSInteger)nodeID withArguments:(NSArray *)arguments;

- (OSCMessage *)n_setMessageWithNodeID:(NSInteger)nodeID andArguments:(NSArray *)arguments;

- (OSCMessage *)s_newMessageWithSynth:(NSString *)synthName
                         andArguments:(NSArray *)arguments
                               nodeID:(NSInteger *)nodeIDDestination
                            addAction:(AKAddAction)addAction
                         targetNodeID:(NSInteger)targetNodeID;

- (OSCMessage *)g_newMessageWithNodeID:(NSInteger *)groupIDDestination;

- (OSCMessage *)b_allocReadMessageWithPath:(NSString *)path 
                              bufferNumber:(NSInteger)bufferNumber;

@end

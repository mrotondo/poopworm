//
//  PWWorm.h
//  PoopWorms
//
//  Created by Mike Rotondo on 5/7/11.
//  Copyright 2011 Stanford. All rights reserved.
//

#import <Foundation/Foundation.h>
#import <QuartzCore/QuartzCore.h>
#import "PWSplotchWorm.h"

@class EWSequence;
@class EWPitchEvent;

@interface PWWorm : NSObject

@property (nonatomic, retain) NSMutableArray* notes;
@property int durationInBeats;
@property BOOL creating;
@property (nonatomic, retain) CAShapeLayer* layer;
@property (nonatomic, retain) PWSplotchWorm* splotchWorm;
@property int beatsSinceLastNote;
@property (nonatomic, retain) EWSequence *sequence;
@property (nonatomic, readonly) long long age;
@property (nonatomic, retain) EWPitchEvent *lastEvent;
// Synthesis Stuffs
@property (nonatomic, retain) NSNumber *groupID;
@property (nonatomic, retain) NSNumber *busID;
@property (nonatomic, retain) NSNumber *outputNodeID;
@property (nonatomic) CGFloat volume;
@property int foodInBelly;
@property int effectInBelly;

@property (nonatomic, readonly) BOOL dead;
@property (nonatomic, readonly) CGRect boundingBox;
@property (nonatomic, assign) BOOL mating;
@property (nonatomic, retain) NSDate *lastDate;

- (id) initWithView:(UIView*)view andAngle:(float)angle;
- (void) addNoteWithPitch:(float)yPercent;
- (void) stopCreating;
//- (void) updatePath;
- (void) clearWorm;
- (void)eatEffect:(NSString *)effectName;
- (void) updateDisplay;

// don't call this
- (void)tick;

@end

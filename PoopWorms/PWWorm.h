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

- (id) initWithView:(UIView*)view;
- (void) addNoteWithPitch:(float)yPercent;
- (void) stopCreating;
//- (void) updatePath;
- (void) clearWorm;

@end

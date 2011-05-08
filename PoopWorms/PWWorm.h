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

@interface PWWorm : NSObject {
    
}

@property (nonatomic, retain) NSMutableArray* notes;
@property int startBeat;
@property int durationInBeats;
@property BOOL creating;
@property (nonatomic, retain) CAShapeLayer* layer;
@property (nonatomic, retain) PWSplotchWorm* splotchWorm;
@property int beatsSinceLastNote;

- (id) initWithView:(UIView*)view;
- (void) addNoteWithPitch:(float)yPercent;
- (void) stopCreating;
//- (void) updatePath;
- (void) clearWorm;
- (void) tick;

@end

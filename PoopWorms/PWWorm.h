//
//  PWWorm.h
//  PoopWorms
//
//  Created by Mike Rotondo on 5/7/11.
//  Copyright 2011 Stanford. All rights reserved.
//

#import <Foundation/Foundation.h>
#import <QuartzCore/QuartzCore.h>

@interface PWWorm : NSObject {
    
}

@property (nonatomic, retain) NSMutableArray* notes;
@property int startBeat;
@property int durationInBeats;
@property BOOL creating;
@property (nonatomic, retain) CAShapeLayer* layer;

- (void) addNoteWithPitch:(float)yPercent;
- (void) stopCreating;
- (void) updatePath;

@end

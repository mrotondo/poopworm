//
//  PWNote.h
//  PoopWorms
//
//  Created by Mike Rotondo on 5/7/11.
//  Copyright 2011 Stanford. All rights reserved.
//

#import <Foundation/Foundation.h>
#import <QuartzCore/QuartzCore.h>
#import "PWTimeEvent.h"

@class PWSplotch;

@interface PWNote : NSObject <PWTimeEvent> {
    
}

@property int beatIndex;
@property int lengthInBeats;
@property float pitchPercent;
@property (nonatomic, retain) CAShapeLayer* layer;
@property (nonatomic, retain) PWSplotch* splotch;

- (id) initWithBeatIndex:(int)beat andPitchPercent:(float)pitchPercent;

@end

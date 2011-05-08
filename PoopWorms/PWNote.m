//
//  PWNote.m
//  PoopWorms
//
//  Created by Mike Rotondo on 5/7/11.
//  Copyright 2011 Stanford. All rights reserved.
//

#import "PWNote.h"


@implementation PWNote
@synthesize beatIndex, lengthInBeats, pitchPercent, layer, splotch;

- (id) initWithBeatIndex:(int)beat andPitchPercent:(float)pitch
{
    self = [super init];
    if (self)
    {
        self.beatIndex = beat;
        self.pitchPercent = pitch;
        self.layer = [CAShapeLayer layer];
        UIBezierPath* path = [UIBezierPath bezierPathWithArcCenter:CGPointMake(10, 10) radius:10 startAngle:0 endAngle:2 * M_PI clockwise:NO];
        self.layer.path = path.CGPath;
        
        self.splotch = nil;
    }
    return self;
}

- (void) makeDo
{
    NSLog(@"JUST TRYIN TO GET BYYYY");
}

@end

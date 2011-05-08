//
//  PWWorm.m
//  PoopWorms
//
//  Created by Mike Rotondo on 5/7/11.
//  Copyright 2011 Stanford. All rights reserved.
//

#import "PWWorm.h"
#import "PWNote.h"

#import "PWMockBeatManager.h"

@implementation PWWorm
@synthesize notes, startBeat, durationInBeats, layer, creating;

- (id) init
{
    self = [super init];
    if (self) {
        self.notes = [NSMutableArray arrayWithCapacity:10];
        self.startBeat = [PWMockBeatManager getBeatClosestToNow];
        self.layer = [CAShapeLayer layer];
        //self.layer.backgroundColor = [UIColor redColor].CGColor;
        self.creating = YES;
    }
    return self;
}

- (void) addNoteWithPitch:(float)pitchPercent
{
    int beatIndex = [PWMockBeatManager getBeatClosestToNow] - self.startBeat;
    PWNote* note = [[PWNote alloc] initWithBeatIndex:beatIndex andPitchPercent:pitchPercent];
    [self.notes addObject:note];
    [self.layer addSublayer:note.layer];
}

- (void) stopCreating
{
    self.durationInBeats = [PWMockBeatManager getBeatClosestToNow] - self.startBeat;
    self.creating = NO;
    
    //NSLog(@"framez %@", NSStringFromCGRect(self.layer.frame));
    CATransform3D transform = CATransform3DMakeScale(0.2, 0.2, 1.0);
    self.layer.transform = transform;
}

- (void) updatePath
{
    if (self.creating)
    {
        int ageInBeats = [PWMockBeatManager getBeatClosestToNow] - self.startBeat;
        int radius = 10;

        UIBezierPath* path = [UIBezierPath bezierPath];
        [path addArcWithCenter:CGPointMake(ageInBeats * radius * 2, 0) radius:20 startAngle:M_PI / 2 endAngle:-M_PI / 2 clockwise:NO];
        CGPoint topPoint = CGPointMake(ageInBeats * radius * 2, -20);
        CGPoint bottomPoint = CGPointMake(ageInBeats * radius * 2, 20);
        CGPoint newTopPoint, newBottomPoint;
        for (PWNote* note in self.notes)
        {
            int xPosition = (ageInBeats - note.beatIndex) * radius * 2;
            int yPosition = 200 * note.pitchPercent;
            newTopPoint = CGPointMake(xPosition, yPosition - 20);
            [path moveToPoint:topPoint];
            
            //[path addLineToPoint:newTopPoint];
            [path addCurveToPoint:newTopPoint controlPoint1:CGPointMake(topPoint.x - 20, topPoint.y) controlPoint2:CGPointMake(newTopPoint.x + 20, newTopPoint.y)];
            
            topPoint = newTopPoint;
            newBottomPoint = CGPointMake(xPosition, yPosition + 20);
            [path moveToPoint:bottomPoint];

            //[path addLineToPoint:newBottomPoint];
            [path addCurveToPoint:newBottomPoint controlPoint1:CGPointMake(bottomPoint.x - 20, bottomPoint.y) controlPoint2:CGPointMake(newBottomPoint.x + 20, newBottomPoint.y)];

            bottomPoint = newBottomPoint;
            note.layer.frame = CGRectMake(xPosition - radius, yPosition - radius, radius * 2, radius * 2);
        }
        self.layer.fillColor = nil;
        self.layer.strokeColor = [UIColor redColor].CGColor;
        self.layer.path = path.CGPath;

        //NSLog(NSStringFromCGRect(path.bounds));
        
        self.layer.frame = path.bounds;
    }
}

@end

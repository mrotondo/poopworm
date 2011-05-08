//
//  PWSplotchHandler.mm
//  TestSplotches
//
//  Created by Nick Kruge on 5/7/11.
//  Copyright 2011 __MyCompanyName__. All rights reserved.
//

#import "PWSplotchHandler.h"
#import "PWSplotch.h"

@implementation PWSplotchHandler

const float density = 30.0;

- (id)initWithView:(UIView*)_view
{
    if ( (self = [super init]) )
    {
        view = _view;
        // seed random
        srand([[NSDate date] timeIntervalSince1970]);
        // an array, for the splotches
        splotchArray = [[NSMutableArray alloc] init];
    }
    return self;
}

- (void)dealloc
{
    [splotchArray release];
    [super dealloc];
}

- (NSString*)getShapeName
{
    int which = rand() % 5;
    switch (which) {
        case 0:
            return @"flare1.png";
            break;
        case 1:
            return @"flare2.png";
            break;
        case 2:
            return @"flare3.png";
            break;
        case 3:
            return @"particle.png";
            break;
        case 4:
            return @"shine2.png";
            break;
        default:
            break;
    }
    return @"flare1.png";
}

- (UIColor*)getShapeColor
{
    int which = rand() % 5;
    switch (which) {
        case 0:
            return [UIColor blueColor];
            break;
        case 1:
            return [UIColor yellowColor];
            break;
        case 2:
            return [UIColor greenColor];
            break;
        case 3:
            return [UIColor redColor];
            break;
        case 4:
            return [UIColor purpleColor];
            break;
        default:
            break;
    }
    return [UIColor blackColor];
}

// splotch nearby detector
- (bool)nothingNearMe:(CGPoint)touchPoint
{
    for (PWSplotch * s in splotchArray)
    {
        if ( (fabs(touchPoint.x - s.center.x) < density) && (fabs(touchPoint.y - s.center.y) < density))
            return NO;
    }
    return YES;
}

// splotch hit detector
- (PWSplotch*)hitSplotch:(CGPoint)wormPoint
{
    for (PWSplotch * s in splotchArray)
    {
        if ( (fabs(wormPoint.x - s.center.x) < 40.0) && (fabs(wormPoint.y - s.center.y) < 40.0))
            return s;
    }
    return nil;
}

- (void)removeSplotch:(PWSplotch*)splotch
{
    [splotchArray removeObject:splotch];
    [splotch removeFromSuperview];
    [splotch release];
}

- (void)handleTouchPoint:(CGPoint)touchPoint
{    
    // TODO: FIX FOOD!
    // perform this randy thing and also make sure nothing is too near, yeah!
    if ( (rand() % 1000) > 0 && [self nothingNearMe:touchPoint] )
    {
        [splotchArray addObject: [[PWSplotch alloc] initWithImageNamed:[self getShapeName] superlayer:view.layer /*superview:view */
                                                                center:touchPoint size:CGSizeMake(40.0,40.0) 
                                                                 color:[self getShapeColor] alpha:1.0 delegate:self]];
    }
}


- (void)handleWormPoint:(CGPoint)_wormPoint
{
    PWSplotch * hit = [self hitSplotch:_wormPoint];
    if ( hit ) 
    {
        [splotchArray removeObject:hit];
        [hit explodeMe];
    }
        //[self removeSplotch:hit];
}

#pragma mark PWSplotchDelegate method(s)
- (void)killMe:(PWSplotch*)splotch
{
    [self removeSplotch:splotch];
}

@end

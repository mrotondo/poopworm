//
//  PWSplotchWorm.m
//  TestSplotches
//
//  Created by Nick Kruge on 5/7/11.
//  Copyright 2011 __MyCompanyName__. All rights reserved.
//

#import "PWSplotchWorm.h"
#import "PWSplotch.h"
#import "EWTiming.h"

@implementation PWSplotchWorm
@synthesize delegate;

- (id)initWithView:(UIView*)_view
{
    if ( (self = [super init]) )
    {
        view = _view;
        // an array, for the splotches
        wormSplotches = [[NSMutableArray alloc] init];
        wormSize = 60.0;
        speed = 0.2;
        moveTime = NO;
    }
    return self;
}

//- (id)retain
//{
//    NSLog(@"callstack %@", [NSThread callStackSymbols]);
//    return [super retain];
//}

- (void)dealloc
{
    for ( PWSplotch * splotch in wormSplotches )
    {
        [splotch removeFromSuperview];
    }
    
    [wormSplotches release];
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

- (UIColor*)getGreenColor
{
    int which = rand() % 5;
    switch (which) {
        case 0:
            return [UIColor colorWithRed:36.0/255.0 green:84.0/255.0 blue:42.0/255.0 alpha:1.0];
            break;
        case 1:
            return [UIColor colorWithRed:46.0/255.0 green:94.0/255.0 blue:47.0/255.0 alpha:1.0];
            break;
        case 2:
            return [UIColor colorWithRed:26.0/255.0 green:82.0/255.0 blue:41.0/255.0 alpha:1.0];
            break;
        case 3:
            return [UIColor colorWithRed:40.0/255.0 green:88.0/255.0 blue:49.0/255.0 alpha:1.0];
            break;
        case 4:
            return [UIColor colorWithRed:36.0/255.0 green:70.0/255.0 blue:36.0/255.0 alpha:1.0];
            break;
        default:
            break;
    }
    return [UIColor blackColor];
}

- (void)move
{
    aniTimer = [[NSTimer scheduledTimerWithTimeInterval:speed target:self selector:@selector(moveForward:) userInfo:nil repeats:YES] retain];
}

- (void)removeSplotch:(PWSplotch*)splotch
{
    [wormSplotches removeObject:splotch];
    [splotch removeFromSuperview];
    [splotch release];
}

- (void)moveForward:(NSTimer*)timer
{
    PWSplotch * splotch = [wormSplotches objectAtIndex:0];
  
    float x = endPoint.x + splotch.center.x - startPoint.x;
    float y = endPoint.y - startPoint.y + splotch.center.y;
    
    // oops i can't figure out fmodf
    if ( x < 0.0 ) x += 768.0;
    if ( x > 768.0 ) x -= 768.0;
    if ( y < 0.0 ) y += 1024.0;
    if ( y > 1024.0 ) y -= 1024.0;
    
    splotch.center = CGPointMake(x,y);
    
    [self.delegate wormHeadLocation:splotch.center];
    //[splotch setImage:[UIImage imageNamed:@"head.png"]];
    
    // now swap to the front
    [wormSplotches removeObjectAtIndex:0];
    //[[wormSplotches lastObject] changeImageTo:@"caterscale.png" withColor:[self getGreenColor]];
    [wormSplotches addObject:splotch];
}

- (bool)jumpTooBigBetween:(CGPoint)pt1 and:(CGPoint)pt2
{
    if ( (fabs(pt1.x - pt2.x) > 700.0) || (fabs(pt1.y - pt2.y) > 900.0) ) return YES;
    return NO;
}

- (void)moveWorm
{
    if ( !moveTime ) return;
    
    PWSplotch * splotch = [wormSplotches objectAtIndex:0];

    float x = endPoint.x + splotch.center.x - startPoint.x;
    float y = endPoint.y - startPoint.y + splotch.center.y;

    // oops i can't figure out fmodf
    if ( x < 0.0 ) x += 768.0;
    if ( x > 768.0 ) x -= 768.0;
    if ( y < 0.0 ) y += 1024.0;
    if ( y > 1024.0 ) y -= 1024.0;
    
    [UIView beginAnimations:nil context:nil];
    [UIView setAnimationCurve:UIViewAnimationCurveLinear];
    [UIView setAnimationDuration: 1.0 / [EWTicker sharedTicker].ticksPerSecond ];
    CGPoint tempCenter = CGPointMake(x, y); 
    for (int i = [wormSplotches count] - 1; i >= 0 ; i--) 
    {
        if ([self jumpTooBigBetween:[[wormSplotches objectAtIndex:i] center] and:tempCenter])
            [UIView setAnimationsEnabled:NO];
        else [UIView setAnimationsEnabled:YES];
        CGPoint tempCenter2 = [[wormSplotches objectAtIndex:i] center];
        [[wormSplotches objectAtIndex:i] setCenter:tempCenter];
        tempCenter = tempCenter2;
    }
    [UIView commitAnimations];
}

- (void)startWorm:(CGPoint)start
{
    PWSplotch * piece = [[[PWSplotch alloc] initWithImageNamed:@"caterscale.png" superview:view 
                                    center:start size:CGSizeMake(wormSize,wormSize) 
                                     color:[self getGreenColor] alpha:1.0 delegate:self]autorelease];
    
    [wormSplotches addObject: piece];
    startPoint = start;

}

- (void)addToWorm:(CGPoint)point tapped:(bool)tapped
{
    PWSplotch* s = [wormSplotches lastObject];
    if ( s.center.x == point.x && s.center.y == point.y ) return;
    
    UIColor * color = (tapped) ? [self getGreenColor] : [UIColor redColor];
    PWSplotch * piece = [[[PWSplotch alloc] initWithImageNamed:@"caterscale.png" superview:view 
                                                        center:point size:CGSizeMake(wormSize,wormSize) 
                                                         color:color alpha:1.0 delegate:self]autorelease];
    
    [wormSplotches addObject: piece];}

- (void)endWorm:(CGPoint)end
{
    PWSplotch * piece = [[[PWSplotch alloc] initWithImageNamed:@"caterscale.png" superview:view 
                                                        center:end size:CGSizeMake(wormSize,wormSize) 
                                                         color:[self getGreenColor] alpha:1.0 delegate:self]autorelease];
    
    [wormSplotches addObject: piece];    
    endPoint = end;
    //[self move];
    moveTime = YES;
}

- (void)stopWorm
{
    if ( [aniTimer isValid] )
    {
        [aniTimer invalidate];
        [aniTimer release];
        aniTimer = nil;
    }
}

@end
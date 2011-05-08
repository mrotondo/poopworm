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
@synthesize delegate, xOffset, yOffset;

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
        
        xOffset = 0.0;
        yOffset = 0.0;
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

- (void)removeSplotch:(PWSplotch*)splotch
{
    [wormSplotches removeObject:splotch];
    [splotch removeFromSuperview];
    [splotch release];
}

- (bool)jumpTooBigBetween:(CGPoint)pt1 and:(CGPoint)pt2
{
    if ( (fabs(pt1.x - pt2.x) > 500.0) || (fabs(pt1.y - pt2.y) > 600.0) ) return YES;
    return NO;
}

- (void)moveWorm
{
    if ( !moveTime ) return;
    
    PWSplotch * splotch = [wormSplotches objectAtIndex:0];

    float x = endPoint.x + splotch.center.x - startPoint.x + xOffset;
    float y = endPoint.y - startPoint.y + splotch.center.y + yOffset;

    xOffset += -5 + 10 * ((float)rand() / RAND_MAX);
    yOffset += -5 + 10 * ((float)rand() / RAND_MAX);
     
    while (x < 0) {
        x += view.bounds.size.width;
    }
    x = fmodf(x, view.bounds.size.width);
    while (y < 0) {
        y += view.bounds.size.height;
    }
    y = fmodf(y, view.bounds.size.height);
    
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
    [self.delegate wormHeadLocation:[[wormSplotches lastObject] center]];
}

- (void)startWorm:(CGPoint)start
{
    PWSplotch * piece = [[[PWSplotch alloc] initWithImageNamed:@"caterscale.png" superview:view 
                                    center:start size:CGSizeMake(wormSize,wormSize) 
                                     color:[UIColor redColor] alpha:1.0 delegate:self]autorelease];
    
    [wormSplotches addObject: piece];
    startPoint = start;

}

- (PWSplotch*)addToWorm:(CGPoint)point tapped:(bool)tapped
{
    PWSplotch* s = [wormSplotches lastObject];
    if ( s.center.x == point.x && s.center.y == point.y ) return nil;
    
    UIColor * color = (tapped) ? [self getGreenColor] : [UIColor redColor];
    PWSplotch * piece = [[[PWSplotch alloc] initWithImageNamed:@"caterscale.png" superview:view 
                                                        center:point size:CGSizeMake(wormSize,wormSize) 
                                                         color:color alpha:1.0 delegate:self]autorelease];
    
    [wormSplotches addObject: piece];
    
    return piece;
}

- (void)endWorm:(CGPoint)end
{
    PWSplotch * piece = [[[PWSplotch alloc] initWithImageNamed:@"caterscale.png" superview:view 
                                                        center:end size:CGSizeMake(wormSize,wormSize) 
                                                         color:[UIColor redColor] alpha:1.0 delegate:self]autorelease];
    
    [wormSplotches addObject: piece];    
    endPoint = end;
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

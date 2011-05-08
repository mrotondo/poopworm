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
#import <QuartzCore/QuartzCore.h>

@interface PWSplotchWorm()

@property int numPathPoints;
- (CGPoint)headCenterInSuperview;

@end

@implementation PWSplotchWorm
@synthesize delegate, wormSplotches, xOffset, yOffset, layer, numPathPoints, scalingFactor, entranceAngle, worm;



- (UIColor*)getBaseColor
{
    int which = rand() % 8;
    switch (which) {
        case 0:
            base[0] = 46.0; base[1] = 94.0; base[2] = 47.0; // green
            break;
        case 1:
            base[0] = 250.0; base[1] = 168.0; base[2] = 25.0; // orangey
            break;
        case 2:
            base[0] = 212.0; base[1] = 66.0; base[2] = 39.0; // dark red
            break;
        case 3:
            base[0] = 0.0; base[1] = 124.0; base[2] = 154.0; // nice blue
            break;
        case 4:
            base[0] = 0.0; base[1] = 90.0; base[2] = 136.0; // darker blue
            break;
        case 5:
            base[0] = 157.0; base[1] = 36.0; base[2] = 158.0; // magenta
            break;
        case 6:
            base[0] = 210.0; base[1] = 31.0; base[2] = 63.0; // green
            break;
        case 7:
            base[0] = 134.0; base[1] = 150.0; base[2] = 58.0; // green
            break;
        default:
            break;
    }
    return [UIColor blackColor];
}


- (id)initWithView:(UIView*)_view andAngle:(float)angle andWorm:(PWWorm*)_worm
{
    if ( (self = [super init]) )
    {
        self.worm = _worm;
        view = _view;
        // an array, for the splotches
        wormSplotches = [[NSMutableArray alloc] init];
        wormSize = 130.0;
        speed = 0.2;
        moveTime = NO;
        
        self.xOffset = 0.0;
        self.yOffset = 0.0;
        
        self.layer = [CAShapeLayer layer];
        self.layer.frame = view.bounds;
        [view.layer addSublayer:self.layer];
        self.layer.transform = CATransform3DMakeRotation(angle, 0, 0, 1.0);
        
        self.entranceAngle = angle;
        self.scalingFactor = 0.2;
        
        [self getBaseColor];
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
    
    [self.layer removeFromSuperlayer];
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

- (UIColor*)getFromBaseColor
{
    float diff0 = (rand() % 1000 / 12.5 ) - 40; // a number between -40 and 40
    float diff1 = (rand() % 1000 / 12.5 ) - 40;
    float diff2 = (rand() % 1000 / 12.5 ) - 40;
    
    return [UIColor colorWithRed:(base[0]+diff0)/255.0 green:(base[1]+diff1)/255.0 blue:(base[2]+diff2)/255.0 alpha:1.0];
}

- (UIColor*)getBrightBaseColor
{    
    return [UIColor colorWithRed:(base[0]+100)/255.0 green:(base[1]+100)/255.0 blue:(base[2]+100)/255.0 alpha:1.0];
}

- (UIColor*)getPathColor
{
    return [self getFromBaseColor];
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

- (CGAffineTransform)inverseTransformForSuperview
{
    CGAffineTransform relativeCenter = CGAffineTransformMakeTranslation(view.bounds.size.width / 2, view.bounds.size.height / 2);
    CGAffineTransform rotatedRelativeCenter = CGAffineTransformRotate(relativeCenter, self.entranceAngle);
    CGAffineTransform scaledRotatedRelativeCenter = CGAffineTransformScale(rotatedRelativeCenter, self.scalingFactor, self.scalingFactor);
    CGAffineTransform scaledRotatedAbsoluteCenter = CGAffineTransformTranslate(scaledRotatedRelativeCenter, -view.bounds.size.width / 2, -view.bounds.size.height / 2);
    return scaledRotatedAbsoluteCenter;
}

- (CGPoint)headCenterInSuperview
{
    CGPoint headCenter = [[wormSplotches lastObject] center];
    return CGPointApplyAffineTransform(headCenter, [self inverseTransformForSuperview]);
}

- (void)moveWorm
{
    if ( !moveTime ) return;
    
    PWSplotch *splotch = [wormSplotches objectAtIndex:0];
    
    float x = endPoint.x + splotch.center.x - startPoint.x + xOffset;
    float y = endPoint.y - startPoint.y + splotch.center.y + yOffset;
    
    xOffset += -5 + 10 * ((float)rand() / RAND_MAX);
    yOffset += -5 + 10 * ((float)rand() / RAND_MAX);
     
//    while (x < 0) {
//        x += view.bounds.size.width;
//    }
//    x = fmodf(x, view.bounds.size.width);
//    while (y < 0) {
//        y += view.bounds.size.height;
//    }
//    y = fmodf(y, view.bounds.size.height);
    
    CGPoint tempCenter = CGPointMake(x, y);
    for (int i = [wormSplotches count] - 1; i >= 0 ; i--)
    {
        CGPoint tempCenter2 = [[wormSplotches objectAtIndex:i] center];
        [[wormSplotches objectAtIndex:i] setCenter:tempCenter];
        tempCenter = tempCenter2;
    }
    
    [self.delegate wormHeadLocation:[self headCenterInSuperview] 
                           withWorm:self.worm];
    
    [self updatePath];
}

- (void) updatePath
{
    if( wormSplotches.count > 0 )
    {
        UIBezierPath *path = [UIBezierPath bezierPath];
        
        [path moveToPoint:[[wormSplotches objectAtIndex:0] center]];
        
        BOOL first = YES;
        for( PWSplotch *splotch in wormSplotches )
        {
            if( !first )
            {
                [path addLineToPoint:splotch.center];
            }
            
            first = NO;
        }
        
        path.lineCapStyle = kCGLineCapRound;
        path.lineJoinStyle = kCGLineJoinRound;
        
        self.layer.fillColor = nil;
        self.layer.lineWidth = 30;
        self.layer.strokeColor = [self getPathColor].CGColor;
        
        self.layer.path = path.CGPath;
    }
    
    return;
    
    if ( [wormSplotches count] > 0 )
    {        
        UIBezierPath* path = [UIBezierPath bezierPath];
        PWSplotch* head = [wormSplotches lastObject];
        [path addArcWithCenter:head.center radius:wormSize startAngle:M_PI / 2 endAngle:-M_PI / 2 clockwise:NO];
        CGPoint topPoint = CGPointMake(head.center.x, head.center.y - wormSize);
        CGPoint newTopPoint, controlPoint1, controlPoint2;
        for (PWSplotch* part in [wormSplotches reverseObjectEnumerator])
        {
            if (part == head)
                continue;
            
            newTopPoint = CGPointMake(part.center.x, part.center.y - wormSize);
            controlPoint1 = CGPointMake(topPoint.x - 4, topPoint.y);
            controlPoint2 = CGPointMake(newTopPoint.x + 4, newTopPoint.y);
            [path addCurveToPoint:newTopPoint controlPoint1:controlPoint1 controlPoint2:controlPoint2];
            
            topPoint = newTopPoint;
        }
        
        PWSplotch* tail = [wormSplotches objectAtIndex:0];
        [path addArcWithCenter:tail.center radius:wormSize startAngle:-M_PI / 2 endAngle:M_PI / 2 clockwise:NO];
        CGPoint bottomPoint = CGPointMake(tail.center.x, tail.center.y + wormSize);
        CGPoint newBottomPoint;
        for (PWSplotch* part in wormSplotches)
        {
            if (part == tail)
                continue;
            
            newBottomPoint = CGPointMake(part.center.x, part.center.y + wormSize);
            controlPoint1 = CGPointMake(bottomPoint.x + 4, bottomPoint.y);
            controlPoint2 = CGPointMake(newBottomPoint.x - 4, newBottomPoint.y);
            [path addCurveToPoint:newBottomPoint controlPoint1:controlPoint1 controlPoint2:controlPoint2];
            
            bottomPoint = newBottomPoint;
        }
        
        self.layer.fillColor = nil;
        self.layer.lineWidth = 4;
        self.layer.strokeColor = [self getFromBaseColor].CGColor;
        
        CABasicAnimation *animation = [CABasicAnimation animationWithKeyPath:@"path"];
        animation.duration = 1.0 / [EWTicker sharedTicker].ticksPerSecond;
        animation.timingFunction = [CAMediaTimingFunction functionWithName:kCAMediaTimingFunctionLinear];
        animation.fromValue = (id)self.layer.path;
        animation.toValue = (id)path.CGPath;
        [self.layer addAnimation:animation forKey:@"animatePath"];
        self.layer.path = path.CGPath;        
    }
}

- (void)startWorm:(CGPoint)start
{
    PWSplotch * piece = [[[PWSplotch alloc] initWithImageNamed:@"caterscale.png" superlayer:self.layer //superview:view 
                                    center:start size:CGSizeMake(wormSize,wormSize) 
                                     color:[self getFromBaseColor] alpha:1.0 delegate:self]autorelease];
    
    [wormSplotches addObject: piece];
    startPoint = start;
    
    [self updatePath];
}

- (PWSplotch*)addToWorm:(CGPoint)point tapped:(bool)tapped
{
    PWSplotch* s = [wormSplotches lastObject];
    if ( s.center.x == point.x && s.center.y == point.y ) return nil;
    
    UIColor * color = (tapped) ? [self getBrightBaseColor] : [self getFromBaseColor];

    PWSplotch * piece = [[[PWSplotch alloc] initWithImageNamed:@"caterscale.png" superlayer:self.layer //superview:view 
                                                        center:point size:CGSizeMake(wormSize,wormSize) 
                                                         color:color alpha:1.0 delegate:self]autorelease];
    
    piece.active = tapped;
    
    [wormSplotches addObject: piece];

    [self updatePath];
    
    return piece;
}

- (void)endWorm:(CGPoint)end
{
    PWSplotch * piece = [[[PWSplotch alloc] initHeadWithImageNamed:@"head.png" superlayer:self.layer //superview:view 
                                                        center:end size:CGSizeMake(wormSize+220.0,wormSize+220.0) 
                                                         color:[self getFromBaseColor] alpha:1.0 delegate:self]autorelease];
    
    [wormSplotches addObject: piece];    
    endPoint = end;
    moveTime = YES;
    [self updatePath];
    
    for (PWSplotch* splotch in wormSplotches)
    {
        splotch.center = CGPointMake(splotch.center.x - 1500, splotch.center.y);
    }
    [UIView beginAnimations:nil context:nil];
    [UIView setAnimationCurve:UIViewAnimationCurveEaseIn];
    [UIView setAnimationDuration: 2.0];
    self.layer.transform = CATransform3DConcat(self.layer.transform, CATransform3DMakeScale(self.scalingFactor, self.scalingFactor, 1.0));
    [UIView commitAnimations];
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

- (void)cleanup
{
    for( PWSplotch *splotch in wormSplotches )
    {
        [self removeSplotch:splotch];
    }
}

- (void)setAlpha:(float)value
{
    for( PWSplotch *splotch in wormSplotches )
    {
        splotch.alpha = value;
    }
}

@end

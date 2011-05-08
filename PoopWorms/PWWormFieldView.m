//
//  PWWormFieldView.m
//  PoopWorms
//
//  Created by Mike Rotondo on 5/7/11.
//  Copyright 2011 Stanford. All rights reserved.
//

#import "PWWormFieldView.h"

@interface PWWormFieldView() {
@private
}
@property (nonatomic, retain) CAShapeLayer* borderLayer;
@property (nonatomic, retain) NSTimer* stopTimer;
- (void) stopCreatingWorm:(NSTimer*)sender;
@end

@implementation PWWormFieldView
@synthesize controller, borderWidth, borderLayer, stopTimer;

- (id)initWithCoder:(NSCoder *)aDecoder
{
    self = [super initWithCoder:aDecoder];
    if (self) {
        // Initialization code
        self.borderWidth = 80;
        self.borderLayer = [CAShapeLayer layer];
        [self.layer addSublayer:self.borderLayer];
        self.borderLayer.fillColor = nil;
        self.borderLayer.lineWidth = 4;
        self.borderLayer.strokeColor = [UIColor blackColor].CGColor;
        
        [self updateBorder];
    }
    return self;
}

- (void) updateBorder
{
    NSLog(@"View bounds: %@", NSStringFromCGRect(self.bounds));
    self.borderLayer.transform = CATransform3DIdentity;
    self.borderLayer.frame = self.bounds;
    UIBezierPath* path = [UIBezierPath bezierPathWithRoundedRect:self.bounds cornerRadius:self.borderWidth/4];
    self.borderLayer.path = path.CGPath;
    float scalingFactorX = (self.bounds.size.width - self.borderWidth * 2) / self.bounds.size.width;
    float scalingFactorY = (self.bounds.size.height - self.borderWidth * 2) / self.bounds.size.height;
    NSLog(@"Scaling factors: %f, %f", scalingFactorX, scalingFactorY);
    self.borderLayer.transform = CATransform3DMakeScale(scalingFactorX, scalingFactorY, 1.0);
}

- (void)touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event
{
    UITouch* touch = [touches anyObject];
    CGPoint loc = [touch locationInView:self];

    if ( !controller.creatingWorm)
    {
        if ([[event allTouches] count] == 1 && (loc.x < self.borderWidth || loc.x > self.bounds.size.width - self.borderWidth || loc.y < self.borderWidth || loc.y > self.bounds.size.height - self.borderWidth))
        {
            float angle = atan2f((self.bounds.size.height - loc.y) - self.bounds.size.height / 2.0, (self.bounds.size.width - loc.x) - self.bounds.size.width / 2.0);
            [controller startCreatingWormWithAngle:angle];
            self.stopTimer = [NSTimer scheduledTimerWithTimeInterval:0.7 target:self selector:@selector(stopCreatingWorm:) userInfo:nil repeats:NO];
        }
    }
    else
    {
        [controller addNoteWithYPercent:loc.y / self.bounds.size.height];
        [self.stopTimer invalidate];
        self.stopTimer = [NSTimer scheduledTimerWithTimeInterval:0.7 target:self selector:@selector(stopCreatingWorm:) userInfo:nil repeats:NO];
    }
}

- (void) stopCreatingWorm:(NSTimer*)sender
{
    [controller stopCreatingWorm]; 
}

- (void)dealloc
{
    [super dealloc];
}

@end

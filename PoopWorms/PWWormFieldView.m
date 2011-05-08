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
@end

@implementation PWWormFieldView
@synthesize controller, borderWidth, borderLayer;

- (id)initWithCoder:(NSCoder *)aDecoder
{
    self = [super initWithCoder:aDecoder];
    if (self) {
        // Initialization code
        self.borderWidth = 50;
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
    UIBezierPath* path = [UIBezierPath bezierPathWithRoundedRect:self.bounds cornerRadius:self.borderWidth];
    self.borderLayer.path = path.CGPath;
    float scalingFactorX = (self.bounds.size.width - self.borderWidth * 2) / self.bounds.size.width;
    float scalingFactorY = (self.bounds.size.height - self.borderWidth * 2) / self.bounds.size.height;
    NSLog(@"Scaling factors: %f, %f", scalingFactorX, scalingFactorY);
    self.borderLayer.transform = CATransform3DMakeScale(scalingFactorX, scalingFactorY, 1.0);
}

// Only override drawRect: if you perform custom drawing.
// An empty implementation adversely affects performance during animation.
/*
- (void)drawRect:(CGRect)rect
{
}
*/

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
        }
    }
    else
    {
        [controller addNoteWithYPercent:loc.y / self.bounds.size.height];
    }
}

- (void)touchesEnded:(NSSet *)touches withEvent:(UIEvent *)event
{
    if ( [[event allTouches] count] == 1 )
    {
        [controller stopCreatingWorm];
    }
}

- (void)touchesCancelled:(NSSet *)touches withEvent:(UIEvent *)event
{
    if ( [[event allTouches] count] == 1 )
    {
        [controller stopCreatingWorm];
    }
}

- (void)dealloc
{
    [super dealloc];
}

@end

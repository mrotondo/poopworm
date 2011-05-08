//
//  PWWormFieldView.m
//  PoopWorms
//
//  Created by Mike Rotondo on 5/7/11.
//  Copyright 2011 Stanford. All rights reserved.
//

#import "PWWormFieldView.h"

@implementation PWWormFieldView
@synthesize controller, borderWidth;

- (id)initWithCoder:(NSCoder *)aDecoder
{
    self = [super initWithCoder:aDecoder];
    if (self) {
        // Initialization code
        self.borderWidth = 50;
        CAShapeLayer* layer = [CAShapeLayer layer];
        UIBezierPath* path = [UIBezierPath bezierPathWithRoundedRect:self.frame cornerRadius:self.borderWidth];
        layer.path = path.CGPath;
        layer.fillColor = nil;
        layer.lineWidth = 4;
        layer.strokeColor = [UIColor blackColor].CGColor;
        float scalingFactorX = (self.bounds.size.width - borderWidth * 2) / self.bounds.size.width;
        float scalingFactorY = (self.bounds.size.height - borderWidth * 2) / self.bounds.size.height;
        NSLog(@"Scaling factors: %f, %f", scalingFactorX, scalingFactorY);
        layer.frame = self.bounds;
        layer.transform = CATransform3DMakeScale(scalingFactorX, scalingFactorY, 1.0);
        [self.layer addSublayer:layer];
    }
    return self;
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

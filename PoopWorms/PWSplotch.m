//
//  PWSplotch.m
//  TestSplotches
//
//  Created by Nick Kruge on 5/7/11.
//  Copyright 2011 __MyCompanyName__. All rights reserved.
//

#import "PWSplotch.h"
#import "QuartzCore/QuartzCore.h"

@implementation PWSplotch
@synthesize delegate;

- (UIImage*)createParticle:(UIImage*)maskImage withColor:(UIColor*)_color
{
    [self setBackgroundColor:_color];
    UIGraphicsBeginImageContext(self.bounds.size);
    [self.layer renderInContext:UIGraphicsGetCurrentContext()];
    UIImage *viewImage = UIGraphicsGetImageFromCurrentImageContext();
    UIGraphicsEndImageContext();
    
    [self setBackgroundColor:[UIColor clearColor]];
    
    CGImageRef maskRef = maskImage.CGImage;
    CGImageRef mask = CGImageMaskCreate(CGImageGetWidth(maskRef),
                                        CGImageGetHeight(maskRef),
                                        CGImageGetBitsPerComponent(maskRef),
                                        CGImageGetBitsPerPixel(maskRef),
                                        CGImageGetBytesPerRow(maskRef),
                                        CGImageGetDataProvider(maskRef), NULL, false);
    
    CGImageRef masked = CGImageCreateWithMask([viewImage CGImage], mask);
    CGImageRelease(mask);
    UIImage* retImage= [UIImage imageWithCGImage:masked];
    CGImageRelease(masked);
    return retImage;
    
}

// override init to include setting the IP and port for OSC
- (id)initWithImageNamed:(NSString*)_imageName superview:(UIView*)sview center:(CGPoint)_center size:(CGSize)_size color:(UIColor*)_color alpha:(float)_alpha delegate:(id)_delegate
{
    if ( (self = [super init]) )
    {
        self.frame = CGRectMake(10.0, 10.0, _size.width, _size.height);
        inAlpha = _alpha;
        self.image = [self createParticle:[UIImage imageNamed:_imageName] withColor:_color];
        [sview addSubview:self];
        self.center = _center;
        //[self animateMe];
        //float randy = rand() % 3000 / 1000.0;
        //[self performSelector:@selector(animateMe) withObject:nil afterDelay:randy];
        delegate = _delegate;
    }
    
    return self;
}

- (void)changeImageTo:(NSString*)_imageName withColor:_color
{
    self.image = nil;
    self.image = [self createParticle:[UIImage imageNamed:_imageName] withColor:_color];
}

- (id)initWithSpecificImageNamed:(NSString*)_imageName superview:(UIView*)sview center:(CGPoint)_center size:(CGSize)_size color:(UIColor*)_color alpha:(float)_alpha delegate:(id)_delegate
{
    if ( (self = [super init]) )
    {
        self.frame = CGRectMake(10.0, 10.0, _size.width, _size.height);
        inAlpha = _alpha;
        self.image = [UIImage imageNamed:_imageName];
        [sview addSubview:self];
        self.center = _center;
        //[self animateMe];
        float randy = rand() % 3000 / 1000.0;
        [self performSelector:@selector(animateMe) withObject:nil afterDelay:randy];
        delegate = _delegate;
    }
    
    return self;
}

- (void) animateMe
{
    self.transform = CGAffineTransformMakeScale(0.6, 0.6);
    [UIView animateWithDuration:0.2
                          delay:0.0
                        options:UIViewAnimationOptionAllowUserInteraction
                     animations:^{ 
                         self.transform = CGAffineTransformConcat(CGAffineTransformMakeScale(1.2, 1.2),CGAffineTransformMakeRotation((rand()%10 - 5)*360.0));
                         
                     } 
                     completion:^(BOOL finished){
                         [UIView animateWithDuration:0.2
                                               delay:0.0
                                             options:UIViewAnimationOptionAllowUserInteraction
                                          animations:^{ 
                                              self.transform = CGAffineTransformMakeScale(1.0, 1.0);
                                          } 
                                          completion:^(BOOL finished){
                                              float randy = rand() % 3000 / 1000.0;
                                              [self performSelector:@selector(animateMe) withObject:nil afterDelay:randy];
                                          }];
                     }];
}

- (void) explodeMe
{
    self.transform = CGAffineTransformMakeScale(0.1, 0.1);
    [UIView animateWithDuration:0.03
                          delay:0.0
                        options:UIViewAnimationOptionAllowUserInteraction
                     animations:^{ 
                         self.transform = CGAffineTransformConcat(CGAffineTransformMakeScale(3.5, 3.5),CGAffineTransformMakeRotation((rand()%10 - 5)*360.0));
                         self.alpha = inAlpha;
                         
                     } 
                     completion:^(BOOL finished){
                         [UIView animateWithDuration:1.0
                                               delay:0.0
                                             options:UIViewAnimationOptionAllowUserInteraction
                                          animations:^{ 
                                              self.transform = CGAffineTransformMakeScale(0.1, 0.1);
                                              self.alpha = 0.0;
                                          } 
                                          completion:^(BOOL finished){
                                              [self.delegate killMe:self];
                                          }];
                     }];
}

- (void) flash
{
    self.transform = CGAffineTransformMakeScale(0.5, 0.5);
    [UIView animateWithDuration:0.03
                          delay:0.0
                        options:UIViewAnimationOptionAllowUserInteraction
                     animations:^{ 
                         self.transform = CGAffineTransformMakeScale(2.0, 2.0);
                     } 
                     completion:^(BOOL finished){
                         [UIView animateWithDuration:0.4
                                               delay:0.0
                                             options:UIViewAnimationOptionAllowUserInteraction
                                          animations:^{ 
                                              self.transform = CGAffineTransformMakeScale(1.0, 1.0);
                                          } 
                                          completion:nil
                          ];
                     }];
}

@end


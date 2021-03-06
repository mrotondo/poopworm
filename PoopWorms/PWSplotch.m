//
//  PWSplotch.m
//  TestSplotches
//
//  Created by Nick Kruge on 5/7/11.
//  Copyright 2011 __MyCompanyName__. All rights reserved.
//

#import "PWSplotch.h"
#import "QuartzCore/QuartzCore.h"

static NSMutableDictionary* imageCache;

@implementation PWSplotch
@synthesize delegate, itemId, isFood, originalColor, originalString, originalImage, flashImage, badge, active;

- (UIImage*)createParticle:(NSString*)maskImageFileName withColor:(UIColor*)_color
{
    if (imageCache == nil) {
        imageCache = [[NSMutableDictionary alloc] initWithCapacity:20];
    }
    
    NSUInteger hash = [maskImageFileName hash] ^ [_color hash];
    UIImage* image = [imageCache objectForKey:[NSNumber numberWithUnsignedInt:hash]];
    if (image != nil)
    {
        return image;
    }
    
    UIImage *maskImage = [UIImage imageNamed:maskImageFileName];
    
    [self setBackgroundColor:_color];
    UIGraphicsBeginImageContext(self.bounds.size);
    [self.layer renderInContext:UIGraphicsGetCurrentContext()];
    UIImage *viewImage = UIGraphicsGetImageFromCurrentImageContext();
    UIGraphicsEndImageContext();
    
    [self setBackgroundColor:[UIColor clearColor]];
    self.itemId = 0;
    self.isFood = YES;
    
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
    
    [imageCache setObject:retImage forKey:[NSNumber numberWithUnsignedInt:hash]];
    
    return retImage;
}


- (UIImage*)createBadge:(NSString*)maskImageFileName withColor:(UIColor*)_color
{
    if (imageCache == nil) {
        imageCache = [[NSMutableDictionary alloc] initWithCapacity:20];
    }
    
    NSUInteger hash = [maskImageFileName hash] ^ [_color hash];
    UIImage* image = [imageCache objectForKey:[NSNumber numberWithUnsignedInt:hash]];
    if (image != nil)
    {
        return image;
    }
    
    UIImage *maskImage = [UIImage imageNamed:maskImageFileName];
    
    self.badge.image = nil;
    [self.badge setBackgroundColor:_color];
    UIGraphicsBeginImageContext(self.bounds.size);
    [self.badge.layer renderInContext:UIGraphicsGetCurrentContext()];
    UIImage *viewImage = UIGraphicsGetImageFromCurrentImageContext();
    UIGraphicsEndImageContext();
    
    [self.badge setBackgroundColor:[UIColor clearColor]];
    
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
    
    [imageCache setObject:retImage forKey:[NSNumber numberWithUnsignedInt:hash]];
    
    return retImage;
}

- (UIColor*)getPurpleColor
{
    int which = rand() % 5;
    switch (which) {
        case 0:
            return [UIColor colorWithRed:81.0/255.0 green:45.0/255.0 blue:130.0/255.0 alpha:1.0];
            break;
        case 1:
            return [UIColor colorWithRed:85.0/255.0 green:30.0/255.0 blue:150.0/255.0 alpha:1.0];
            break;
        case 2:
            return [UIColor colorWithRed:90.0/255.0 green:15.0/255.0 blue:120.0/255.0 alpha:1.0];
            break;
        case 3:
            return [UIColor colorWithRed:80.0/255.0 green:30.0/255.0 blue:145.0/255.0 alpha:1.0];
            break;
        case 4:
            return [UIColor colorWithRed:90.0/255.0 green:30.0/255.0 blue:140.0/255.0 alpha:1.0];
            break;
        default:
            break;
    }
    return [UIColor blackColor];
}

// override init to include setting the IP and port for OSC
- (id)initWithImageNamed:(NSString*)_imageName superlayer:(CALayer*)layer /*superview:(UIView*)sview*/ center:(CGPoint)_center size:(CGSize)_size color:(UIColor*)_color alpha:(float)_alpha delegate:(id)_delegate
{
    if ( (self = [super init]) )
    {
        self.frame = CGRectMake(10.0, 10.0, _size.width, _size.height);
        inAlpha = _alpha;
        
        self.flashImage = [self createParticle:_imageName withColor:[self getPurpleColor]];
        self.image = [self createParticle:_imageName withColor:_color];
        
        //[sview addSubview:self];
        [layer addSublayer:self.layer];
        
        CAShapeLayer* ringLayer = [CAShapeLayer layer];
        UIBezierPath* ringPath = [UIBezierPath bezierPathWithArcCenter:CGPointMake(_size.width / 2, _size.height / 2) radius:1.5 * _size.width / 2 startAngle:0 endAngle:2 * M_PI clockwise:NO];
        //ringLayer.lineWidth = 4;
        ringLayer.fillColor = CGColorCreateCopyWithAlpha(_color.CGColor, 0.2); 
        ringLayer.path = ringPath.CGPath;
        ringLayer.strokeColor = nil;
        [self.layer addSublayer:ringLayer];
        
        self.originalColor = _color;
        self.originalString = _imageName;
        self.originalImage = self.image;
        
        self.badge = [[UIImageView alloc] initWithFrame:self.frame];
        [self addSubview:self.badge];
        self.badge.transform = CGAffineTransformMakeScale(0.6, 0.6);
        self.badge.center = CGPointMake(self.frame.size.width/2.0, self.frame.size.height/2.0);
        
        self.center = _center;
        //[self animateMe];
        //float randy = rand() % 3000 / 1000.0;
        //[self performSelector:@selector(animateMe) withObject:nil afterDelay:randy];
        delegate = _delegate;
        self.active = NO;
        isHead = NO;
    }
    
    return self;
}

// override init to include setting the IP and port for OSC
- (id)initHeadWithImageNamed:(NSString*)_imageName superlayer:(CALayer*)layer /*superview:(UIView*)sview*/ center:(CGPoint)_center size:(CGSize)_size color:(UIColor*)_color alpha:(float)_alpha delegate:(id)_delegate
{
    if ( (self = [super init]) )
    {
        self.frame = CGRectMake(10.0, 10.0, _size.width, _size.height);
        inAlpha = _alpha;
        
        self.flashImage = [self createParticle:@"headopen.png" withColor:_color];
        self.image = [self createParticle:_imageName withColor:_color];
        
        //[sview addSubview:self];
        [layer addSublayer:self.layer];
        
//        CAShapeLayer* ringLayer = [CAShapeLayer layer];
//        UIBezierPath* ringPath = [UIBezierPath bezierPathWithArcCenter:CGPointMake(_size.width / 2, _size.height / 2) radius:1.5 * _size.width / 2 startAngle:0 endAngle:2 * M_PI clockwise:NO];
//        //ringLayer.lineWidth = 4;
//        ringLayer.fillColor = CGColorCreateCopyWithAlpha(_color.CGColor, 0.2); 
//        ringLayer.path = ringPath.CGPath;
//        ringLayer.strokeColor = nil;
//        [self.layer addSublayer:ringLayer];
        
        self.originalColor = _color;
        self.originalString = _imageName;
        self.originalImage = self.image;
        
        self.badge = nil;
        
        self.center = _center;
        //[self animateMe];
        //float randy = rand() % 3000 / 1000.0;
        //[self performSelector:@selector(animateMe) withObject:nil afterDelay:randy];
        delegate = _delegate;
        self.active = NO;
        isHead = YES;
    }
    
    return self;
}

- (void)dealloc
{
    self.originalColor = nil;
    self.originalString = nil;
    self.originalImage = nil;
    self.flashImage = nil;
    [self.badge removeFromSuperview];
    self.badge = nil;
    [super dealloc];
}

- (void)changeImageTo:(NSString*)_imageName withColor:_color
{
    self.image = [self createParticle:_imageName withColor:_color];
}

- (void)changeImageTo:(NSString*)_imageName all:(bool)all
{
    // BAIL OUT IF ALREADY THAT IMAGE! YEAH!
    if ( isHead ) [self animateMe];
    if ( [_imageName isEqualToString:self.originalString] ) return;
    
    self.originalString = _imageName;
    //self.originalImage = [self createParticle:_imageName withColor:[UIColor whiteColor]];
    //self.flashImage = [self createParticle:_imageName withColor:[self getPurpleColor]];
    if (all && !self.active) self.badge.image = [self createBadge:_imageName withColor:[UIColor whiteColor]];
    else if ( !all && self.active ) self.badge.image = [self createBadge:_imageName withColor:[UIColor whiteColor]];
}

- (void) animateMe
{
    self.transform = CGAffineTransformMakeScale(0.6, 0.6);
    [UIView animateWithDuration:0.2
                          delay:0.0
                        options:UIViewAnimationOptionAllowUserInteraction
                     animations:^{ 
                         self.transform = CGAffineTransformMakeScale(2.0, 2.0);
                         self.image = self.flashImage;
                         
                     } 
                     completion:^(BOOL finished){
                         [UIView animateWithDuration:0.2
                                               delay:0.0
                                             options:UIViewAnimationOptionAllowUserInteraction
                                          animations:^{ 
                                              self.transform = CGAffineTransformMakeScale(1.0, 1.0);
                                              //self.image = self.originalImage;
                                          } 
                                          completion:^(BOOL finished){
                                              //float randy = rand() % 3000 / 1000.0;
                                              //[self performSelector:@selector(animateMe) withObject:nil afterDelay:randy];
                                              self.image = self.originalImage;
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
                         self.transform = CGAffineTransformMakeScale(3.0, 3.0);
                         //self.image = self.flashImage;
                     } 
                     completion:^(BOOL finished){
                         [UIView animateWithDuration:0.4
                                               delay:0.0
                                             options:UIViewAnimationOptionAllowUserInteraction
                                          animations:^{ 
                                              self.transform = CGAffineTransformMakeScale(1.0, 1.0);
                                              

                                          } 
                                          completion:^(BOOL finished){
                                              //self.image = self.originalImage;
                                          }
                          ];
                     }];
}

@end


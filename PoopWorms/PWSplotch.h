//
//  PWSplotch.h
//  TestSplotches
//
//  Created by Nick Kruge on 5/7/11.
//  Copyright 2011 __MyCompanyName__. All rights reserved.
//

#import <Foundation/Foundation.h>

@protocol PWSplotchDelegate;

@interface PWSplotch : UIImageView {
    id <PWSplotchDelegate> delegate;
    float inAlpha;
}

@property (nonatomic,assign) id <PWSplotchDelegate> delegate;
- (id)initWithImageNamed:(NSString*)_imageName superview:(UIView*)sview center:(CGPoint)_center size:(CGSize)_size color:(UIColor*)_color alpha:(float)_alpha delegate:(id)_delegate;
- (id)initWithSpecificImageNamed:(NSString*)_imageName superview:(UIView*)sview center:(CGPoint)_center size:(CGSize)_size color:(UIColor*)_color alpha:(float)_alpha delegate:(id)_delegate;

- (void)changeImageTo:(NSString*)_imageName withColor:_color;

- (void)animateMe;
- (void)explodeMe;
- (void)flash;


@end

@protocol PWSplotchDelegate
- (void)killMe:(PWSplotch*)splotch;

@end
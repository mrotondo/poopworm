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
    bool isHead;
    
}

@property (nonatomic,assign) id <PWSplotchDelegate> delegate;
@property (nonatomic,retain) UIColor * originalColor;
@property (nonatomic,retain) NSString * originalString;
@property (nonatomic,retain) UIImage * originalImage;
@property (nonatomic,retain) UIImage * flashImage;
@property (nonatomic,retain) UIImageView * badge;
@property bool active;


@property int itemId;
@property bool isFood;

- (id)initWithImageNamed:(NSString*)_imageName superlayer:(CALayer*)layer /*superview:(UIView*)sview*/ center:(CGPoint)_center size:(CGSize)_size color:(UIColor*)_color alpha:(float)_alpha delegate:(id)_delegate;
- (id)initHeadWithImageNamed:(NSString*)_imageName superlayer:(CALayer*)layer /*superview:(UIView*)sview*/ center:(CGPoint)_center size:(CGSize)_size color:(UIColor*)_color alpha:(float)_alpha delegate:(id)_delegate;

- (void)changeImageTo:(NSString*)_imageName withColor:_color;
- (void)changeImageTo:(NSString*)_imageName all:(bool)all;
- (void)animateMe;
- (void)explodeMe;
- (void)flash;


@end

@protocol PWSplotchDelegate
- (void)killMe:(PWSplotch*)splotch;

@end
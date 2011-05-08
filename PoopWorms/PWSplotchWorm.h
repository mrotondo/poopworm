//
//  PWSplotchWorm.h
//  TestSplotches
//
//  Created by Nick Kruge on 5/7/11.
//  Copyright 2011 __MyCompanyName__. All rights reserved.
//

#import <Foundation/Foundation.h>
@class PWSplotch;
@class CAShapeLayer;

@protocol PWSplotchWormDelegate

- (void)wormHeadLocation:(CGPoint)head;

@end

@interface PWSplotchWorm : NSObject {
    NSMutableArray * wormSplotches;
    UIView * view;
    float wormSize;
    float speed;
    CGPoint startPoint, endPoint;
    NSTimer *aniTimer;
    
    bool moveTime;
}

@property (nonatomic,assign) id <PWSplotchWormDelegate>delegate;
@property float xOffset;
@property float yOffset;
@property (nonatomic, retain) CAShapeLayer* layer;
@property float scalingFactor;
@property float entranceAngle;

- (id)initWithView:(UIView*)_view andAngle:(float)angle;

- (void)removeSplotch:(PWSplotch*)splotch;
- (void)startWorm:(CGPoint)start;
- (PWSplotch*)addToWorm:(CGPoint)point tapped:(bool)tapped;
- (void)endWorm:(CGPoint)end;

- (void)stopWorm;
- (void)updatePath;
- (void)moveWorm;

- (void)cleanup;

@end

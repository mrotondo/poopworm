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
@class PWWorm;

@protocol PWSplotchWormDelegate

- (void)wormHeadLocation:(CGPoint)head withWorm:(PWWorm*)worm;

@end

@interface PWSplotchWorm : NSObject {
    UIView * view;
    float wormSize;
    float speed;
    CGPoint startPoint, endPoint;
    NSTimer *aniTimer;
    
    bool moveTime;
}

@property (nonatomic,assign) id <PWSplotchWormDelegate>delegate;
@property (nonatomic, readonly) NSMutableArray *wormSplotches;
@property float xOffset;
@property float yOffset;
@property (nonatomic, retain) CAShapeLayer* layer;
@property float scalingFactor;
@property float entranceAngle;
@property (nonatomic, retain) PWWorm* worm;

- (id)initWithView:(UIView*)_view andAngle:(float)angle andWorm:(PWWorm*)worm;

- (void)removeSplotch:(PWSplotch*)splotch;
- (void)startWorm:(CGPoint)start;
- (PWSplotch*)addToWorm:(CGPoint)point tapped:(bool)tapped;
- (void)endWorm:(CGPoint)end;

- (void)stopWorm;
- (void)updatePath;
- (void)moveWorm;

- (void)cleanup;

- (CGAffineTransform)extracted_method;

@end

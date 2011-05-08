//
//  PWSplotchWorm.h
//  TestSplotches
//
//  Created by Nick Kruge on 5/7/11.
//  Copyright 2011 __MyCompanyName__. All rights reserved.
//

#import <Foundation/Foundation.h>
@class PWSplotch;

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

- (id)initWithView:(UIView*)_view;

- (void)removeSplotch:(PWSplotch*)splotch;
- (void)startWorm:(CGPoint)start;
- (void)addToWorm:(CGPoint)point tapped:(bool)tapped;
- (void)endWorm:(CGPoint)end;

- (void)stopWorm;

- (void)moveWorm;

@end

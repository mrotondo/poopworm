//
//  PWSplotchHandler.h
//  TestSplotches
//
//  Created by Nick Kruge on 5/7/11.
//  Copyright 2011 __MyCompanyName__. All rights reserved.
//

#import <Foundation/Foundation.h>


@interface PWSplotchHandler : NSObject {
    UIView * view; // pointer to the view we will post to
    NSMutableArray * splotchArray;
}

- (id)initWithView:(UIView*)_view;
- (void)handleTouchPoint:(CGPoint)_touchPoint;
- (void)handleWormPoint:(CGPoint)_wormPoint;

@end

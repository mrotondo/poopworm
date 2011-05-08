//
//  PWMockBeatManager.h
//  PoopWorms
//
//  Created by Mike Rotondo on 5/7/11.
//  Copyright 2011 Stanford. All rights reserved.
//

#import <Foundation/Foundation.h>


@interface PWMockBeatManager : NSObject {
    
}

+ (int)getBeatClosestToNow;
+ (void)incrementBeat;

@end

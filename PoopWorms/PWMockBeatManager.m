//
//  PWMockBeatManager.m
//  PoopWorms
//
//  Created by Mike Rotondo on 5/7/11.
//  Copyright 2011 Stanford. All rights reserved.
//

#import "PWMockBeatManager.h"

int i = 0;

@implementation PWMockBeatManager

+ (int)getBeatClosestToNow
{
    return i;
}

+ (void)incrementBeat
{
    i++;
}

@end

//
//  SoundFood.m
//  PoopWorms
//
//  Created by Mike Rotondo on 5/8/11.
//  Copyright 2011 Eeoo. All rights reserved.
//

#import "SoundFood.h"


@implementation SoundFood

+ synthNameForFoodId:(int)foodId
{
    NSArray *foodNames = [NSArray arrayWithObjects:
                          @"BasicSine",
                          @"BasicPulse",
                          @"BasicSaw",
                          @"ResonNoise",
                          @"WavyPulse",
                          nil];
    return [foodNames objectAtIndex:foodId];
}

+ imageForFoodId:(int)foodId
{
    NSArray *images = [NSArray arrayWithObjects:
                          @"circle.png",
                          @"square.png",
                          @"triangle.png",
                          @"noise.png",
                          @"wave.png",
                          nil];
    return [images objectAtIndex:foodId];
}

@end

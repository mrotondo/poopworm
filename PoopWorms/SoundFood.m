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
                          @"EmptyBelly",
                          @"BasicSine",
                          @"BasicPulse",
                          @"BasicSaw",
                          @"ResonNoise",
                          @"WavyPulse",
                          @"SineKick",
                          nil];
    return [foodNames objectAtIndex:foodId];
}

+ imageForFoodId:(int)foodId
{
    NSArray *images = [NSArray arrayWithObjects:
                       @"blank.png",
                       @"circle.png",
                       @"square.png",
                       @"triangle.png",
                       @"noise.png",
                       @"wavypulse.png",
                       @"kick.png",
                       nil];
    return [images objectAtIndex:foodId];
}


+ effectNameForEffectId:(int)effectId
{
    NSArray *effectNames = [NSArray arrayWithObjects:
                            @"Tanh",
                            @"PitchShift",
                            @"Flanger",
                            @"CombNDelay",
                            nil];
    return [effectNames objectAtIndex:effectId];
}

+ imageForEffectId:(int)effectId
{
    NSArray *images = [NSArray arrayWithObjects:
                       @"distortion.png",
                       @"pitch.png",
                       @"flange.png",
                       @"echo.png",
                       nil];
    return [images objectAtIndex:effectId];
}


@end

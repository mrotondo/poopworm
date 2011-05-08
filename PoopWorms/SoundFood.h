//
//  SoundFood.h
//  PoopWorms
//
//  Created by Mike Rotondo on 5/8/11.
//  Copyright 2011 Eeoo. All rights reserved.
//

#import <Foundation/Foundation.h>


@interface SoundFood : NSObject {
    
}

+ synthNameForFoodId:(int)foodId;
+ imageForFoodId:(int)foodId;

+ effectNameForEffectId:(int)effectId;
+ imageForEffectId:(int)effectId;
@end

//
//  CALayer+Additions.m
//  Explor
//
//  Created by Luke Iannini on 5/6/11.
//  Copyright 2011 Eeoo. All rights reserved.
//

#import "CALayer+Additions.h"


@implementation CALayer (CALayer_Additions)

- (void)hc_removeAnimations
{
    NSMutableDictionary *newActions = [NSMutableDictionary dictionaryWithObjectsAndKeys:
                                       [NSNull null], @"onOrderIn",
                                       [NSNull null], @"onOrderOut",
                                       [NSNull null], @"sublayers",
                                       [NSNull null], @"contents",
                                       [NSNull null], @"bounds",
                                       [NSNull null], @"center",
                                       nil];
    self.actions = newActions;
}

@end

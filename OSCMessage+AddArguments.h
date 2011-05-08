//
//  OSCMessage+AddArguments.h
//  Artikulator
//
//  Created by Luke Iannini on 7/4/10.
//  Copyright 2010 Hello, Chair Inc. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "VVOSC.h"

@interface OSCMessage (AddArguments)

- (void)addArguments:(NSArray *)arguments;
- (NSString *)OSCString;

@end

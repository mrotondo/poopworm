//
//  PWWormFieldView.m
//  PoopWorms
//
//  Created by Mike Rotondo on 5/7/11.
//  Copyright 2011 Stanford. All rights reserved.
//

#import "PWWormFieldView.h"


@implementation PWWormFieldView
@synthesize controller;

- (id)initWithFrame:(CGRect)frame
{
    self = [super initWithFrame:frame];
    if (self) {
        // Initialization code
    }
    return self;
}

// Only override drawRect: if you perform custom drawing.
// An empty implementation adversely affects performance during animation.
- (void)drawRect:(CGRect)rect
{
    [self.controller drawWorms];
}

- (void)touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event
{
    if ( [[event allTouches] count] == 1 )
    {
        [controller startCreatingWorm];
    }
    else if ( controller.creatingWorm)
    {
        UITouch* touch = [touches anyObject];
        [controller addNoteWithYPercent:[touch locationInView:self].y / self.frame.size.height];
    }
}

- (void)touchesEnded:(NSSet *)touches withEvent:(UIEvent *)event
{
    if ( [[event allTouches] count] == 1 )
    {
        [controller stopCreatingWorm];
    }
}

- (void)touchesCancelled:(NSSet *)touches withEvent:(UIEvent *)event
{
    if ( [[event allTouches] count] == 1 )
    {
        [controller stopCreatingWorm];
    }
}

- (void)dealloc
{
    [super dealloc];
}

@end

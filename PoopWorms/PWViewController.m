//
//  PoopWormsViewController.m
//  PoopWorms
//
//  Created by Mike Rotondo on 5/7/11.
//  Copyright 2011 Stanford. All rights reserved.
//

#import "PWViewController.h"
#import "QuartzCore/QuartzCore.h"
#import "PWWormFieldView.h"
#import "EWTiming.h"

@implementation PWViewController
@synthesize creatingWorm, currentWorm, worms;

- (void)dealloc
{
    [super dealloc];
}

- (void)didReceiveMemoryWarning
{
    // Releases the view if it doesn't have a superview.
    [super didReceiveMemoryWarning];
    
    // Release any cached data, images, etc that aren't in use.
}

#pragma mark - View lifecycle


// Implement viewDidLoad to do additional setup after loading the view, typically from a nib.
- (void)viewDidLoad
{
    [super viewDidLoad];

    self.worms = [NSMutableArray arrayWithCapacity:10];
    
    ((PWWormFieldView*) self.view).controller = self;
    
    NSTimer* mockTimer = [NSTimer timerWithTimeInterval:0.1
                                                 target:self
                                               selector:@selector(tick:)
                                               userInfo:nil
                                                repeats:YES];
    [[NSRunLoop currentRunLoop] addTimer:mockTimer forMode:NSDefaultRunLoopMode];
}

- (void) tick:(NSTimer*)sender
{
//    for (PWWorm* worm in self.worms)
//    {
//        [worm tick];
//    }
    
    [self.view setNeedsDisplay];
}

- (void) startCreatingWorm
{
    self.creatingWorm = YES;
    self.currentWorm = [[[PWWorm alloc] initWithView:self.view] autorelease];
    [self.worms addObject:self.currentWorm];
    [self.view.layer addSublayer:self.currentWorm.layer];
}

- (void) stopCreatingWorm
{
    self.creatingWorm = NO;
    [self.currentWorm stopCreating];
    self.currentWorm = nil;
}

- (void) addNoteWithYPercent:(float)yPercent
{
    if (self.creatingWorm)
    {
        [self.currentWorm addNoteWithPitch:yPercent];
    }
}

- (void) drawWorms
{
//    for (PWWorm* worm in self.worms)
//    {
//        [worm updatePath];
//    }
}

- (void)viewDidUnload
{
    [super viewDidUnload];
    // Release any retained subviews of the main view.
    // e.g. self.myOutlet = nil;
}

- (IBAction) clearStuff
{
    for ( PWWorm * worm in self.worms )
        [worm clearWorm];
    
    [self.worms removeAllObjects];
    
}

- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation
{
    // Return YES for supported orientations
    return YES;
}

@end

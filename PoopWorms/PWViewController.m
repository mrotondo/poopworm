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
#import "PWSplotchHandler.h"
#import "EWTiming.h"
#import "AKSCSynth.h"

@implementation PWViewController
@synthesize CPULabel;
@synthesize creatingWorm, currentWorm, worms, splotchHandler;

- (void)dealloc
{
    [CPULabel release];
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
    self.splotchHandler = [[PWSplotchHandler alloc] initWithView:self.view];
    
    ((PWWormFieldView*) self.view).controller = self;
    
    [NSTimer scheduledTimerWithTimeInterval:1 target:self selector:@selector(updateCPU:) userInfo:nil repeats:YES];
}

- (void)updateCPU:(NSTimer*)sender
{
    self.CPULabel.text = [NSString stringWithFormat:@"^%.02f, ~%.02f", 
                          [[AKSCSynth sharedSynth] peakCPU],
                          [[AKSCSynth sharedSynth] averageCPU],
                          nil];
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
    [self setCPULabel:nil];
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

- (void) touchesMoved:(NSSet *)touches withEvent:(UIEvent *)event
{
    if ( [[event allTouches] count] == 1 )
    {
        CGPoint touchPoint = [[touches anyObject] locationInView:self.view];
        [self.splotchHandler handleTouchPoint:touchPoint];
    }
}

// HERE'S WHERE THE WORM HEAD GETS HANDLED AND THINGS GET EATEN
- (void)wormHeadLocation:(CGPoint)head
{
    [splotchHandler handleWormPoint:head];
}

@end

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
@synthesize creatingWorm, currentWorm, worms, splotchHandler, currentFoodId;

- (void)dealloc
{
    [CPULabel release];
    [super dealloc];
}

- (void)didRotateFromInterfaceOrientation:(UIInterfaceOrientation)fromInterfaceOrientation
{
    [((PWWormFieldView*) self.view) updateBorder];
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
    
    [NSTimer scheduledTimerWithTimeInterval:1 
                                     target:self 
                                   selector:@selector(updateCPU:) 
                                   userInfo:nil 
                                    repeats:YES];
    
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(tick) name:tickNotification object:nil];
}

- (IBAction)postTreeAction:(id)sender
{
    [[AKSCSynth sharedSynth] dumpTree];
}

- (void)updateCPU:(NSTimer*)sender
{
    self.CPULabel.text = [NSString stringWithFormat:@"^%.02f, ~%.02f", 
                          [[AKSCSynth sharedSynth] peakCPU],
                          [[AKSCSynth sharedSynth] averageCPU],
                          nil];
}

- (void) startCreatingWormWithAngle:(float)angle
{
    self.creatingWorm = YES;
    self.currentWorm = [[[PWWorm alloc] initWithView:self.view andAngle:angle] autorelease];
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

- (void)viewDidUnload
{
    [[NSNotificationCenter defaultCenter] removeObserver:self];
    [self setCPULabel:nil];
    
    [super viewDidUnload];
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
    CGPoint loc = [[touches anyObject] locationInView:self.view];
    PWWormFieldView* wormView = (PWWormFieldView*) self.view;
    if ( [[event allTouches] count] == 1  && !(loc.x < wormView.borderWidth || loc.x > wormView.bounds.size.width - wormView.borderWidth || loc.y < wormView.borderWidth || loc.y > wormView.bounds.size.height - wormView.borderWidth))
    {

        [self.splotchHandler handleTouchPoint:loc withFoodId:self.currentFoodId];
    }
}

- (IBAction) selectSoundFood:(UIButton*)sender
{
    self.currentFoodId = sender.tag;
}

// HERE'S WHERE THE WORM HEAD GETS HANDLED AND THINGS GET EATEN
- (void)wormHeadLocation:(CGPoint)head withWorm:(PWWorm*)worm
{
    [splotchHandler handleWormPoint:head withWorm:worm];
}

- (void)tick
{
    // remove the dead worms
    for( PWWorm *worm in [[self.worms copy] autorelease] )
    {
        if( worm.dead && !worm.creating )
            [self.worms removeObject:worm];
    }
}

@end

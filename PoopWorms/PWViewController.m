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
@synthesize foodButtons;
@synthesize CPULabel;
@synthesize creatingWorm, currentWorm, worms, splotchHandler, currentFoodId, currentEffectId, placingFood;

- (void)dealloc
{
    [CPULabel release];
    [foodButtons release];
    [super dealloc];
}

- (void)willAnimateSecondHalfOfRotationFromInterfaceOrientation:(UIInterfaceOrientation)fromInterfaceOrientation duration:(NSTimeInterval)duration
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
    self.splotchHandler = [[[PWSplotchHandler alloc] initWithView:self.view] autorelease];
    
    ((PWWormFieldView*) self.view).controller = self;
    
    self.currentFoodId = 1;
    self.placingFood = YES;
    
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
    
    [self setFoodButtons:nil];
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
    if (!self.creatingWorm)
    {
        CGPoint loc = [[touches anyObject] locationInView:self.view];
        PWWormFieldView* wormView = (PWWormFieldView*) self.view;
        if ( [[event allTouches] count] == 1  && !(loc.x < wormView.borderWidth || loc.x > wormView.bounds.size.width - wormView.borderWidth || loc.y < wormView.borderWidth || loc.y > wormView.bounds.size.height - wormView.borderWidth))
        {
            int itemId = self.currentFoodId;
            if (!self.placingFood)
                itemId = self.currentEffectId;
            
            [self.splotchHandler handleTouchPoint:loc withItemId:itemId isFood:self.placingFood];
        }
    }
}

- (void)unhighlightAllButtons
{
    for (UIButton *button in self.foodButtons) 
    {
        button.layer.borderWidth = 0;
    }
}

- (IBAction) selectSoundFood:(UIButton*)sender
{
    [self unhighlightAllButtons];
    sender.layer.borderColor = [UIColor orangeColor].CGColor;
    sender.layer.borderWidth = 5;
    self.currentFoodId = sender.tag;
    self.placingFood = YES;
}


- (IBAction) selectEffect:(UIButton*)sender
{
    [self unhighlightAllButtons];
    sender.layer.borderColor = [UIColor orangeColor].CGColor;
    sender.layer.borderWidth = 5;
    self.currentEffectId = sender.tag;
    self.placingFood = NO;
}

// HERE'S WHERE THE WORM HEAD GETS HANDLED AND THINGS GET EATEN
- (void)wormHeadLocation:(CGPoint)head withWorm:(PWWorm*)worm
{
    [splotchHandler handleWormPoint:head withWorm:worm];
}

- (PWWorm *)offspringOf:(PWWorm *)mother and:(PWWorm *)father
{
    if( arc4random() % 2 == 0 )
        return nil;
    
    float angle = (arc4random() % 1000) / 1000.0 * M_PI * 2;
    int lengthDiff = abs( mother.sequence.length - father.sequence.length );
    int minLength = MIN( mother.sequence.length, father.sequence.length );
    
    int newLength = minLength + (lengthDiff > 0 ? arc4random() % lengthDiff : 0);
    
    PWWorm *worm = [[[PWWorm alloc] initWithView:self.view andAngle:angle] autorelease];
    for( int i = 0; i < newLength; i++ )
    {
        [worm.sequence tick];
        [worm tick];
        
        EWPitchEvent *motherEvent = [[mother.sequence eventsAtTick:i] anyObject];
        EWPitchEvent *fatherEvent = [[mother.sequence eventsAtTick:i] anyObject];
        
        float pitch = -1;
        
        if( motherEvent && arc4random() % 2 == 0 )
            pitch = motherEvent.pitch;
        
        if( fatherEvent && arc4random() % 3 == 0 )
            pitch = fatherEvent.pitch;
        
        if( pitch != -1 )
            [worm addNoteWithPitch:pitch];
    }
    [worm stopCreating];
    
    if( arc4random() % 2 == 0 && mother.effectInBelly )
    {
        worm.effectInBelly = mother.effectInBelly;
        [worm eatEffect:mother.activeEffectName];
    }
    else if( father.effectInBelly )
    {
        worm.effectInBelly = father.effectInBelly;
        [worm eatEffect:father.activeEffectName];
    }
    
    return worm;
}

- (void)tick
{
    // remove the dead worms
    for( PWWorm *worm in [[self.worms copy] autorelease] )
    {
        if( worm.dead && !worm.creating )
            [self.worms removeObject:worm];
    }
    
    for( int i = 0; i < self.worms.count; i++ )
    {
        PWWorm *worm1 = [self.worms objectAtIndex:i];
        CGRect bbox1 = worm1.boundingBox;
        bbox1 = CGRectApplyAffineTransform( bbox1, [worm1.splotchWorm inverseTransformForSuperview] );
        
        BOOL foundMate = NO;
        
        for( int j = i + 1; j < self.worms.count; j++ )
        {
            PWWorm *worm2 = [self.worms objectAtIndex:j];
            CGRect bbox2 = worm2.boundingBox;
            bbox2 = CGRectApplyAffineTransform( bbox2, [worm2.splotchWorm inverseTransformForSuperview] );
  
            if( CGRectIntersectsRect( bbox1, bbox2 ) )
            {
                if( !worm1.mating || !worm2.mating &&
                    -[worm1.lastDate timeIntervalSinceNow] > 3 &&
                    -[worm2.lastDate timeIntervalSinceNow] > 3 )
                {
//                    NSLog( @"I would mate %@ and %@", worm1, worm2 );
                    
                    PWWorm *baby = [self offspringOf:worm1 and:worm2];
                    
                    if( baby )
                    {
                        [self.worms addObject:baby];
                        [self.view.layer addSublayer:baby.layer];
                        
                        worm1.lastDate = [NSDate date];
                        worm2.lastDate = [NSDate date];
                    }
                }
                
                worm1.mating = YES;
                worm2.mating = YES;
                foundMate = YES;
            }
        }
        
        if( !foundMate )
            worm1.mating = NO;
    }
}

@end

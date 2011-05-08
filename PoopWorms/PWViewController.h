//
//  PoopWormsViewController.h
//  PoopWorms
//
//  Created by Mike Rotondo on 5/7/11.
//  Copyright 2011 Stanford. All rights reserved.
//

#import <UIKit/UIKit.h>
#import "PWWorm.h"

@interface PWViewController : UIViewController <UIGestureRecognizerDelegate> {
}

@property BOOL creatingWorm;
@property (nonatomic, retain) PWWorm* currentWorm;
@property (nonatomic, retain) NSMutableSet* worms;

- (void) startCreatingWorm;
- (void) stopCreatingWorm;
- (void) addNoteWithYPercent:(float)yPercent;
- (void) drawWorms;
- (void) tick:(NSTimer*)sender;

- (IBAction) clearStuff;

@end

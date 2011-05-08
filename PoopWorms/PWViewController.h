//
//  PoopWormsViewController.h
//  PoopWorms
//
//  Created by Mike Rotondo on 5/7/11.
//  Copyright 2011 Stanford. All rights reserved.
//

#import <UIKit/UIKit.h>
#import "PWWorm.h"
@class PWSplotchHandler;


@interface PWViewController : UIViewController <PWSplotchWormDelegate> {
    UILabel *CPULabel;
}

@property (nonatomic, retain) IBOutlet UILabel *CPULabel;
@property BOOL creatingWorm;
@property (nonatomic, retain) PWWorm* currentWorm;
@property (nonatomic, retain) NSMutableSet* worms;
@property (nonatomic, retain) PWSplotchHandler * splotchHandler;

- (void) startCreatingWorm;
- (void) stopCreatingWorm;
- (void) addNoteWithYPercent:(float)yPercent;
- (void) drawWorms;
- (void) tick:(NSTimer*)sender;

- (IBAction) clearStuff;

@end

//
//  PoopWormsAppDelegate.m
//  PoopWorms
//
//  Created by Mike Rotondo on 5/7/11.
//  Copyright 2011 Stanford. All rights reserved.
//

#import "PoopWormsAppDelegate.h"
#import "PWViewController.h"
#import "EWTiming.h"
#import "AKSCSynth.h"

@implementation PoopWormsAppDelegate

@synthesize window=_window;

@synthesize viewController=_viewController;

- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions
{
    // Override point for customization after application launch.
     
    self.window.rootViewController = self.viewController;
    [self.window makeKeyAndVisible];
    
    [[EWTicker sharedTicker] start];
    [AKSCSynth sharedSynth]; // luke says this makes sure the audio is ready but tom is skeptical
    
//    EWSequence *seq = [EWSequence new];
//    seq.length = 16;
//    [seq addEvent:[[EWPitchEvent alloc] initWithPitch:0.5] atTick:0];
//    [seq addEvent:[[EWPitchEvent alloc] initWithPitch:0] atTick:4];
//    [seq addEvent:[[EWPitchEvent alloc] initWithPitch:0] atTick:8];
//    [seq addEvent:[[EWPitchEvent alloc] initWithPitch:0] atTick:12];
//    [seq play];
    
    return YES;
}

- (void)applicationWillResignActive:(UIApplication *)application
{
    /*
     Sent when the application is about to move from active to inactive state. This can occur for certain types of temporary interruptions (such as an incoming phone call or SMS message) or when the user quits the application and it begins the transition to the background state.
     Use this method to pause ongoing tasks, disable timers, and throttle down OpenGL ES frame rates. Games should use this method to pause the game.
     */
}

- (void)applicationDidEnterBackground:(UIApplication *)application
{
    /*
     Use this method to release shared resources, save user data, invalidate timers, and store enough application state information to restore your application to its current state in case it is terminated later. 
     If your application supports background execution, this method is called instead of applicationWillTerminate: when the user quits.
     */
}

- (void)applicationWillEnterForeground:(UIApplication *)application
{
    /*
     Called as part of the transition from the background to the inactive state; here you can undo many of the changes made on entering the background.
     */
}

- (void)applicationDidBecomeActive:(UIApplication *)application
{
    /*
     Restart any tasks that were paused (or not yet started) while the application was inactive. If the application was previously in the background, optionally refresh the user interface.
     */
}

- (void)applicationWillTerminate:(UIApplication *)application
{
    /*
     Called when the application is about to terminate.
     Save data if appropriate.
     See also applicationDidEnterBackground:.
     */
}

- (void)dealloc
{
    [_window release];
    [_viewController release];
    [super dealloc];
}

@end

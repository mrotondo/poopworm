
#import <UIKit/UIKit.h>
#import "OSCNode.h"




#define AddressSpaceUpdateMenus @"AddressSpaceUpdateMenus"









//	OSCAddressSpace delegate protocol
@protocol OSCAddressSpaceDelegateProtocol
- (void) newNodeCreated:(OSCNode *)n;
@end




@interface OSCAddressSpace : OSCNode {
	id			delegate;
}

+ (OSCAddressSpace *) mainSpace;
+ (void) refreshMenu;


- (void) renameAddress:(NSString *)before to:(NSString *)after;
- (void) renameAddressArray:(NSArray *)before toArray:(NSArray *)after;

- (void) setNode:(OSCNode *)n forAddress:(NSString *)a;
- (void) setNode:(OSCNode *)n forAddressArray:(NSArray *)a;

//	this method is called whenever a new node is added to the address space- subclasses can override this for custom notifications
- (void) newNodeCreated:(OSCNode *)n;

//	unlike a normal node: first finds the destination node, then dispatches the msg
- (void) dispatchMessage:(OSCMessage *)m;

- (void) addDelegate:(id)d forPath:(NSString *)p;
- (void) removeDelegate:(id)d forPath:(NSString *)p;

@property (assign, readwrite) id delegate;


@end

//
//  ARInterfaceFeedbackViewController.h
//  ApplianceReader
//

#import <UIKit/UIKit.h>

@class ARInterface;


#pragma mark - Constants

typedef NS_ENUM(NSInteger, ARInterfaceFeedbackViewControllerMode)
{
    ARInterfaceFeedbackViewControllerUndefinedMode = 0,
    ARInterfaceFeedbackViewControllerFeedbackMode,
    ARInterfaceFeedbackViewControllerCaptureMode,
    ARInterfaceFeedbackViewControllerRecaptureMode
};



#pragma mark - Interface

@interface ARInterfaceFeedbackViewController : UIViewController

@property (nonatomic) ARInterface *interface;
@property (nonatomic) ARInterfaceFeedbackViewControllerMode mode;

@end

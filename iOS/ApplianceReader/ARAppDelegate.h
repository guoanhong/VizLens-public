//
//  ARAppDelegate.h
//  ApplianceReader
//

#import <UIKit/UIKit.h>

#import "ARInterfaceFeedbackViewController.h"


#pragma mark - Helpers

NS_INLINE UIColor *blueColor()
{
    return [UIColor colorWithRed:(38.0f / 255.0f) green:(139.0f / 255.0f) blue:(210.0f / 255.0f) alpha:1.0f];
}

NS_INLINE UIColor *greenColor()
{
    return [UIColor colorWithRed:(133.0f / 255.0f) green:(153.0f / 255.0f) blue:(0.0f / 255.0f) alpha:1.0f];
}

NS_INLINE BOOL isOKHTTPURLResponse(NSURLResponse *urlResponse)
{
    return [urlResponse isKindOfClass:[NSHTTPURLResponse class]] && ([(NSHTTPURLResponse *)urlResponse statusCode] == 200);
}

NS_INLINE UIColor *redColor()
{
    return [UIColor colorWithRed:(220.0f / 255.0f) green:(50.0f / 255.0f) blue:(47.0f / 255.0f) alpha:1.0f];
}

NS_INLINE UIColor *selectionColor()
{
    return [UIColor colorWithRed:(147.0f / 255.0f) green:(161.0f / 255.0f) blue:(161.0f / 255.0f) alpha:1.0f];
}

NS_INLINE UIColor *yellowColor()
{
    return [UIColor colorWithRed:(181.0f / 255.0f) green:(137.0f / 255.0f) blue:(0.0f / 255.0f) alpha:1.0f];
}


#pragma mark - Interface

@interface ARAppDelegate : UIResponder <UIApplicationDelegate>

@property (strong, nonatomic) UIWindow *window;

// Settings.
@property (nonatomic) float feedbackDelay;
@property (nonatomic) float imageCompression;
@property (nonatomic, copy) NSString *imageSize;
@property (nonatomic) BOOL polite;
@property (nonatomic, copy) NSString *serverDomain;
@property (nonatomic) BOOL sound;
@property (nonatomic) float uploadDelay;
@property (nonatomic, copy) NSString *userID;

@end

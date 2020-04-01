//
//  ARAppDelegate.m
//  ApplianceReader
//

#import "ARAppDelegate.h"

@import AVFoundation;

#ifdef AR_DEV
#import "ARDevViewController.h"
#else
#import "ARInterfaceListTableViewController.h"
#endif


#pragma mark - Globals

static NSString *ARFeedbackDelayKey = @"ARFeedbackDelayKey";
static NSString *ARImageCompressionKey = @"ARImageCompressionKey";
static NSString *ARImageSizeKey = @"ARImageSizeKey";
static NSString *ARPoliteKey = @"ARPoliteKey";
static NSString *ARServerDomainKey = @"ARServerDomainKey";
static NSString *ARSoundKey = @"ARSoundKey";
static NSString *ARUploadDelayKey = @"ARUploadDelayKey";
static NSString *ARUserIDKey = @"ARUserIDKey";


#pragma mark - Implementation

@implementation ARAppDelegate

- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions
{
    UIWindow *window = [[UIWindow alloc] initWithFrame:[[UIScreen mainScreen] bounds]];
#ifdef AR_DEV
    ARDevViewController *devViewController = [[ARDevViewController alloc] initWithNibName:@"ARDevViewController" bundle:nil];
    window.rootViewController = devViewController;
#else
    ARInterfaceListTableViewController *interfaceListTableViewController = [[ARInterfaceListTableViewController alloc] initWithNibName:@"ARInterfaceListTableViewController" bundle:nil];
    window.rootViewController = interfaceListTableViewController;
#endif
    [window makeKeyAndVisible];
    self.window = window;
    return YES;
}


#pragma mark - Settings

- (BOOL)polite
{
    return [[NSUserDefaults standardUserDefaults] boolForKey:ARPoliteKey];
}

- (void)setPolite:(BOOL)polite
{
    [[NSUserDefaults standardUserDefaults] setBool:polite forKey:ARPoliteKey];
}

- (BOOL)sound
{
    return [[NSUserDefaults standardUserDefaults] boolForKey:ARSoundKey];
}

- (void)setSound:(BOOL)sound
{
    [[NSUserDefaults standardUserDefaults] setBool:sound forKey:ARSoundKey];
}

- (float)imageCompression
{
    float imageCompression = [[NSUserDefaults standardUserDefaults] floatForKey:ARImageCompressionKey];
    BOOL isUndefinedImageCompression = ((imageCompression < 0.1) || (imageCompression > 1.0));
    return (isUndefinedImageCompression) ? 0.8 : imageCompression;
}

- (void)setImageCompression:(float)imageCompression
{
    [[NSUserDefaults standardUserDefaults] setFloat:imageCompression forKey:ARImageCompressionKey];
}

- (NSString *)imageSize
{
    NSString *imageSize = (NSString *)[[NSUserDefaults standardUserDefaults] objectForKey:ARImageSizeKey];
    return (imageSize) ? imageSize : AVCaptureSessionPreset1280x720;
}

- (void)setImageSize:(NSString *)imageSize
{
    if (imageSize)
    {
        [[NSUserDefaults standardUserDefaults] setObject:imageSize forKey:ARImageSizeKey];
    }
}

- (float)feedbackDelay
{
    float feedbackDelay = [[NSUserDefaults standardUserDefaults] floatForKey:ARFeedbackDelayKey];
    BOOL isUndefinedFeedbackDelay = ((feedbackDelay < 0.1) || (feedbackDelay > 1.0));
    return (isUndefinedFeedbackDelay) ? 0.4 : feedbackDelay;
}

- (void)setFeedbackDelay:(float)feedbackDelay
{
    [[NSUserDefaults standardUserDefaults] setFloat:feedbackDelay forKey:ARFeedbackDelayKey];
}

- (NSString *)serverDomain
{
    NSString *serverDomain = (NSString *)[[NSUserDefaults standardUserDefaults] objectForKey:ARServerDomainKey];
    return (serverDomain) ? serverDomain : @"52.7.12.229/VizLensDynamic";
}

- (void)setServerDomain:(NSString *)serverDomain
{
    if (serverDomain)
    {
        [[NSUserDefaults standardUserDefaults] setObject:serverDomain forKey:ARServerDomainKey];
    }
}

- (float)uploadDelay
{
    float uploadDelay = [[NSUserDefaults standardUserDefaults] floatForKey:ARUploadDelayKey];
    BOOL isUndefinedUploadDelay = ((uploadDelay < 0.1) || (uploadDelay > 1.0));
    return (isUndefinedUploadDelay) ? 0.4 : uploadDelay;
}

- (void)setUploadDelay:(float)uploadDelay
{
    [[NSUserDefaults standardUserDefaults] setFloat:uploadDelay forKey:ARUploadDelayKey];
}

- (NSString *)userID
{
    NSString *userID = (NSString *)[[NSUserDefaults standardUserDefaults] objectForKey:ARUserIDKey];
    return (userID) ? userID : @"user";
}

- (void)setUserID:(NSString *)userID
{
    if (userID)
    {
        [[NSUserDefaults standardUserDefaults] setObject:userID forKey:ARUserIDKey];
    }
}

@end

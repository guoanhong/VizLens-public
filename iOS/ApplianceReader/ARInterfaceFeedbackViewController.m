//
//  ARInterfaceFeedbackViewController.m
//  ApplianceReader
//

#import "ARInterfaceFeedbackViewController.h"

@import AVFoundation;

#import "ARAppDelegate.h"
#import "ARCollectionViewInterfaceLayout.h"
#import "ARInterface.h"
#import "ARTarget.h"
#import "ARTargetCollectionViewCell.h"


#pragma mark - Constants

static const NSString *ARHeaderBoundaryString = @"3Z67HFJT965CRK93KQRB";


#pragma mark - Globals

static NSString *ARTargetCollectionViewCellReuseIdentifier = @"ARTargetCollectionViewCellReuseIdentifier";


#pragma mark - Helpers

NS_INLINE NSString *filenameWithPrefix(NSString *prefix, NSString *name)
{
    NSDateFormatter *dateFormatter = [[NSDateFormatter alloc] init];
    [dateFormatter setDateFormat:@"hh:mm:ss.SSSa-MMM-dd-yy"];
    return [NSString stringWithFormat:@"%@_%@_%@", prefix, [dateFormatter stringFromDate:[NSDate date]], name];
}

NS_INLINE BOOL isCaptureMode(ARInterfaceFeedbackViewControllerMode mode)
{
    return (mode == ARInterfaceFeedbackViewControllerCaptureMode);
}

NS_INLINE BOOL isFeedbackMode(ARInterfaceFeedbackViewControllerMode mode)
{
    return (mode == ARInterfaceFeedbackViewControllerFeedbackMode);
}

NS_INLINE BOOL isRecaptureMode(ARInterfaceFeedbackViewControllerMode mode)
{
    return (mode == ARInterfaceFeedbackViewControllerRecaptureMode);
}

NS_INLINE BOOL isNoFingerFeedback(NSString *feedback)
{
    return [feedback isEqualToString:@"no finger"];
}

NS_INLINE BOOL isNoObjectFeedback(NSString *feedback)
{
    return [feedback isEqualToString:@"no object"];
}

NS_INLINE CGSize normalizedContentSizeForContentSizeArray(NSArray *contentSizeArray)
{
    return CGSizeMake([(NSString *)contentSizeArray[0] floatValue], [(NSString *)contentSizeArray[1] floatValue]);
}

NS_INLINE NSArray *targetsForTargetArrays(NSArray *targetArrays)
{
    NSMutableArray *targets = nil;
    if (targetArrays.count > 0)
    {
        targets = [NSMutableArray array];
        for (NSArray *targetArray in targetArrays)
        {
            [targets addObject:[ARTarget targetWithTargetArray:targetArray]];
        }
    }
    return [targets copy];
}

NS_INLINE AVCaptureVideoOrientation videoOrientationForDeviceOrientation(UIDeviceOrientation deviceOrientation)
{
    switch (deviceOrientation)
    {
        case UIDeviceOrientationPortrait:
            return AVCaptureVideoOrientationPortrait;
        case UIDeviceOrientationPortraitUpsideDown:
            return AVCaptureVideoOrientationPortraitUpsideDown;
        case UIDeviceOrientationLandscapeLeft:
            return AVCaptureVideoOrientationLandscapeRight;
        case UIDeviceOrientationLandscapeRight:
            return AVCaptureVideoOrientationLandscapeLeft;
        default:
            return AVCaptureVideoOrientationLandscapeLeft;
    }
}


#pragma mark - Private Interface

@interface ARInterfaceFeedbackViewController () <ARCollectionViewInterfaceLayoutDataSource, AVCaptureVideoDataOutputSampleBufferDelegate, UICollectionViewDataSource, UICollectionViewDelegate>

// Capture session.
@property (nonatomic) AVCaptureDevice *captureDevice;
@property (nonatomic) AVCaptureSession *captureSession;
@property (nonatomic) dispatch_queue_t captureSessionDispatchQueue;
@property (nonatomic) AVCaptureStillImageOutput *captureStillImageOutput;
@property (nonatomic) AVCaptureVideoPreviewLayer *captureVideoPreviewLayer;

// Other.
@property (nonatomic, readonly) NSString *captureSessionPreset;
@property (nonatomic) CGSize normalizedCollectionViewContentSize;
@property (nonatomic) NSArray *targets;

// Speech.
@property (nonatomic) NSString *politeFeedback;

// State.
@property (nonatomic) NSString *stateIdString;

// Sound.
@property (nonatomic) SystemSoundID noFingerSoundID;
@property (nonatomic) SystemSoundID noObjectSoundID;

// Queue.
@property (nonatomic, readonly) dispatch_source_t feedbackDispatchSource;
@property (nonatomic) BOOL isGETingFeedback;
@property (nonatomic) BOOL isGETingTargets;
@property (nonatomic) BOOL isPOSTingVideo;
@property (nonatomic, readonly) dispatch_source_t refreshTargetTableDispatchSource;

// User interface.
@property (nonatomic, weak) IBOutlet UIButton *captureButton;
@property (nonatomic, weak) IBOutlet UIButton *exitButton;
@property (nonatomic, weak) IBOutlet UILabel *feedbackLabel;
@property (nonatomic, weak) IBOutlet UICollectionView *targetCollectionView;

- (IBAction)capture:(UIButton *)captureButton;
- (IBAction)exit:(UIButton *)exitButton;

@end


#pragma mark - Implementation

@implementation ARInterfaceFeedbackViewController

- (void)dealloc
{
    [[NSNotificationCenter defaultCenter] removeObserver:self];
    
    AudioServicesDisposeSystemSoundID(self.noFingerSoundID);
    AudioServicesDisposeSystemSoundID(self.noObjectSoundID);
}

- (BOOL)prefersStatusBarHidden
{
    return YES;
}

- (void)viewDidAppear:(BOOL)animated
{
    [super viewDidAppear:animated];
    
    // Adjust the video size if necessary.
    self.captureSession.sessionPreset = self.captureSessionPreset;
    
    [CATransaction begin];
    [CATransaction setDisableActions:YES];
    self.captureVideoPreviewLayer.frame = self.view.bounds;
    AVCaptureConnection *previewLayerConnection=self.captureVideoPreviewLayer.connection;
    if ([previewLayerConnection isVideoOrientationSupported])
        [previewLayerConnection setVideoOrientation:[[UIApplication sharedApplication] statusBarOrientation]];
    [CATransaction commit];
    
    // State Id string initialize to -
    self.stateIdString = @"-";
    
    // Request access to the camera and start the capture session.
    switch ([AVCaptureDevice authorizationStatusForMediaType:AVMediaTypeVideo])
    {
        case AVAuthorizationStatusNotDetermined:
        {
            ARInterfaceFeedbackViewController * __weak weakSelf = self;
            [AVCaptureDevice requestAccessForMediaType:AVMediaTypeVideo completionHandler:^(BOOL granted) {
                if (granted)
                {
                    // Starting the capture session makes user interface
                    // changes that need to happen on the main thread.
                    dispatch_async(dispatch_get_main_queue(), ^{
                        [weakSelf.captureSession startRunning];
                    });
                }
            }];
            break;
        }
        case AVAuthorizationStatusAuthorized:
        {
            [self.captureSession startRunning];
            break;
        }
        default:
        {
            // TODO: Handle failure gracefully.
            break;
        }
    }
}

- (void)viewDidDisappear:(BOOL)animated
{
    [super viewDidDisappear:animated];
    
    ARInterfaceFeedbackViewControllerMode mode = self.mode;
    // Stop generating feedback and refreshing the targets if necessary.
    if (isFeedbackMode(mode))
    {
        dispatch_suspend(self.feedbackDispatchSource);
        
        // dispatch_suspend(self.refreshTargetTableDispatchSource);
    }
    
    [self.captureSession stopRunning];
}

- (void)viewDidLoad
{
    [super viewDidLoad];
    
    UICollectionView *targetCollectionView = self.targetCollectionView;
    [(ARCollectionViewInterfaceLayout *)targetCollectionView.collectionViewLayout setInterfaceLayoutDataSource:self];
    
    [targetCollectionView registerNib:[UINib nibWithNibName:@"ARTargetCollectionViewCell" bundle:nil]
           forCellWithReuseIdentifier:ARTargetCollectionViewCellReuseIdentifier];
    
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(didFinishFeedback:)
                                                 name:UIAccessibilityAnnouncementDidFinishNotification
                                               object:nil];
    
    // Create 'no finger' sound.
    NSURL *noFingerSoundURL = [[NSBundle mainBundle] URLForResource:@"no_finger" withExtension:@"aif"];
    SystemSoundID noFingerSoundID;
    AudioServicesCreateSystemSoundID((__bridge CFURLRef)noFingerSoundURL, &noFingerSoundID);
    self.noFingerSoundID = noFingerSoundID;
    
    // Create 'no object' sound.
    NSURL *noObjectSoundURL = [[NSBundle mainBundle] URLForResource:@"no_object" withExtension:@"aif"];
    SystemSoundID noObjectSoundID;
    AudioServicesCreateSystemSoundID((__bridge CFURLRef)noObjectSoundURL, &noObjectSoundID);
    self.noObjectSoundID = noObjectSoundID;
    
    AVCaptureSession *captureSession = self.captureSession;
    if (!captureSession)
    {
        // Create and configure the capture session.
        captureSession = [[AVCaptureSession alloc] init];
        self.captureSession = captureSession;
        
        // Create and configure the dispatch queue.
        dispatch_queue_t captureSessionDispatchQueue = dispatch_queue_create("ARCaptureSessionDispatchQueueLabel", DISPATCH_QUEUE_SERIAL);
        self.captureSessionDispatchQueue = captureSessionDispatchQueue;
        
        // Create and configure the capture session input.
        AVCaptureDevice *captureDevice = [AVCaptureDevice defaultDeviceWithMediaType:AVMediaTypeVideo];
        AVCaptureDeviceInput *captureDeviceInput = [AVCaptureDeviceInput deviceInputWithDevice:captureDevice error:nil];
        if ([captureSession canAddInput:captureDeviceInput])
        {
            [captureSession addInput:captureDeviceInput];
        }
        self.captureDevice = captureDevice;
        
        // Create and configure the capture session video output.
        AVCaptureVideoDataOutput *captureVideoDataOutput = [[AVCaptureVideoDataOutput alloc] init];
        captureVideoDataOutput.alwaysDiscardsLateVideoFrames = YES;
        captureVideoDataOutput.videoSettings = @{(__bridge NSString *)kCVPixelBufferPixelFormatTypeKey : @(kCVPixelFormatType_32BGRA)};
        [captureVideoDataOutput setSampleBufferDelegate:self queue:captureSessionDispatchQueue];
        if ([captureSession canAddOutput:captureVideoDataOutput])
        {
            [captureSession addOutput:captureVideoDataOutput];
        }
        
        // Create and configure the capture session image output.
        AVCaptureStillImageOutput *captureStillImageOutput = [[AVCaptureStillImageOutput alloc] init];
        captureStillImageOutput.outputSettings = @{AVVideoCodecKey : AVVideoCodecJPEG};
        if ([captureSession canAddOutput:captureStillImageOutput])
        {
            [captureSession addOutput:captureStillImageOutput];
        }
        self.captureStillImageOutput = captureStillImageOutput;
        
        // Create and configure the capture session preview.
        AVCaptureVideoPreviewLayer *captureVideoPreviewLayer = [AVCaptureVideoPreviewLayer layerWithSession:captureSession];
        captureVideoPreviewLayer.videoGravity = AVLayerVideoGravityResizeAspectFill;
        [self.view.layer insertSublayer:captureVideoPreviewLayer atIndex:0];
        self.captureVideoPreviewLayer = captureVideoPreviewLayer;
    }
}

- (void)viewWillAppear:(BOOL)animated
{
    [super viewWillAppear:animated];
    
    ARInterfaceFeedbackViewControllerMode mode = self.mode;
    self.captureButton.hidden = !isCaptureMode(mode) && !isRecaptureMode(mode);
    self.feedbackLabel.hidden = !isFeedbackMode(mode);
    self.targetCollectionView.hidden = !isFeedbackMode(mode);
    
    self.isGETingFeedback = NO;
    self.isGETingTargets = NO;
    self.isPOSTingVideo = NO;
    
    self.feedbackLabel.text = nil;
    self.normalizedCollectionViewContentSize = CGSizeZero;
    self.targets = nil;
    
    [self.targetCollectionView reloadData];
    
    // Start generating feedback and refreshing the targets if necessary.
    if (isFeedbackMode(mode))
    {
        dispatch_source_t feedbackDispatchSource = self.feedbackDispatchSource;
        ARAppDelegate *appDelegate = (ARAppDelegate *)[UIApplication sharedApplication].delegate;
        float feedbackDelay = appDelegate.feedbackDelay;
        dispatch_source_set_timer(feedbackDispatchSource, DISPATCH_TIME_NOW, feedbackDelay * NSEC_PER_SEC, feedbackDelay * 0.1 * NSEC_PER_SEC);
        dispatch_resume(feedbackDispatchSource);
        
        // dispatch_resume(self.refreshTargetTableDispatchSource);
    }
}


#pragma mark - Other

- (NSString *)captureSessionPreset
{
    ARInterfaceFeedbackViewControllerMode mode = self.mode;
    if (isCaptureMode(mode))
    {
        return AVCaptureSessionPreset1920x1080;
    }
    else
    {
        ARAppDelegate *appDelegate = (ARAppDelegate *)[UIApplication sharedApplication].delegate;
        return appDelegate.imageSize;
    }
}

@synthesize feedbackDispatchSource = _feedbackDispatchSource;

- (dispatch_source_t)feedbackDispatchSource
{
    if (!_feedbackDispatchSource)
    {
        dispatch_source_t feedbackDispatchSource = dispatch_source_create(DISPATCH_SOURCE_TYPE_TIMER, 0, 0, dispatch_get_main_queue());
        ARInterfaceFeedbackViewController * __weak weakSelf = self;
        dispatch_source_set_event_handler(feedbackDispatchSource, ^{
            if (!weakSelf.isGETingFeedback)
            {
                weakSelf.isGETingFeedback = YES;
                
                // Create and configure a new url request.
                NSMutableURLRequest *urlRequest = [[NSMutableURLRequest alloc] init];
                urlRequest.HTTPMethod = @"POST";
                // URL.
                ARAppDelegate *appDelegate = (ARAppDelegate *)[UIApplication sharedApplication].delegate;
                NSString *interfaceUploadURLString = [NSString stringWithFormat:@"http://%@/php/getFeedback.php", appDelegate.serverDomain];
                urlRequest.URL = [NSURL URLWithString:interfaceUploadURLString];
                // HTTP header.
                NSString *httpHeaderContentTypeString = [NSString stringWithFormat:@"multipart/form-data; boundary=%@", ARHeaderBoundaryString];
                [urlRequest setValue:httpHeaderContentTypeString forHTTPHeaderField: @"Content-Type"];
                // HTTP body.
                NSMutableData *httpBody = [NSMutableData data];
                [httpBody appendData:[[NSString stringWithFormat:@"--%@\r\n", ARHeaderBoundaryString] dataUsingEncoding:NSUTF8StringEncoding]];
                [httpBody appendData:[@"Content-Disposition: form-data; name=\"userid\"\r\n\r\n" dataUsingEncoding:NSUTF8StringEncoding]];
                [httpBody appendData:[[NSString stringWithFormat:@"%@\r\n", appDelegate.userID] dataUsingEncoding:NSUTF8StringEncoding]];
                [httpBody appendData:[[NSString stringWithFormat:@"--%@--\r\n", ARHeaderBoundaryString] dataUsingEncoding:NSUTF8StringEncoding]];
                urlRequest.HTTPBody = httpBody;
                
                // Get feedback.
                [[[NSURLSession sharedSession] dataTaskWithRequest:urlRequest completionHandler:^(NSData * _Nullable data, NSURLResponse * _Nullable response, NSError * _Nullable error) {
                    if (data)
                    {
                        NSString *stateIdFeedbackString = [[NSString alloc] initWithData:data encoding:NSUTF8StringEncoding];
                        
                        NSArray *stateIdFeedbackStringComponents = [stateIdFeedbackString componentsSeparatedByString:@"|"];
                        
                        NSLog(@"stateIdFeedbackString: %@", stateIdFeedbackString);
                        
                        NSString *stateIdStringTmp = stateIdFeedbackStringComponents[0];
                        if (stateIdStringTmp.length) {
                            self.stateIdString = stateIdFeedbackStringComponents[0];
                        }
                        
                        NSString *feedbackString = stateIdFeedbackStringComponents[1];
                        
                        if ([feedbackString  isEqual: @"exitinterface"]) {
                            [self.exitButton sendActionsForControlEvents:UIControlEventTouchUpInside];
                        }
                        
                        // If the feedback string has substance, use it.
                        if (feedbackString.length)
                        {
                            // Update feedback on the main thread.
                            dispatch_async(dispatch_get_main_queue(), ^{
                                // This workaround is a fragile way to know if VoiceOver is speaking.
                                BOOL isSpeakingFeedback = [[AVAudioSession sharedInstance] isOtherAudioPlaying];
                                BOOL isPolite = appDelegate.polite;
                                if (isSpeakingFeedback && isPolite)
                                {
                                    weakSelf.politeFeedback = feedbackString;
                                }
                                else
                                {
                                    weakSelf.feedbackLabel.text = feedbackString;
                                    
                                    if (appDelegate.sound && isNoFingerFeedback(feedbackString))
                                    {
                                        AudioServicesPlaySystemSound(weakSelf.noFingerSoundID);
                                    }
                                    else if (appDelegate.sound && isNoObjectFeedback(feedbackString))
                                    {
                                        AudioServicesPlaySystemSound(weakSelf.noObjectSoundID);
                                    }
                                    else
                                    {
                                        UIAccessibilityPostNotification(UIAccessibilityAnnouncementNotification, feedbackString);
                                    }
                                }
                            });
                        }
                    }
                    
                    weakSelf.isGETingFeedback = NO;
                }] resume];
            }
        });
        _feedbackDispatchSource = feedbackDispatchSource;
    }
    return _feedbackDispatchSource;
}

@synthesize refreshTargetTableDispatchSource = _refreshTargetTableDispatchSource;

- (dispatch_source_t)refreshTargetTableDispatchSource
{
    if (!_refreshTargetTableDispatchSource)
    {
        dispatch_source_t refreshTargetTableDispatchSource = dispatch_source_create(DISPATCH_SOURCE_TYPE_TIMER, 0, 0, dispatch_get_main_queue());
        ARInterfaceFeedbackViewController * __weak weakSelf = self;
        dispatch_source_set_event_handler(refreshTargetTableDispatchSource, ^{
            if (!weakSelf.isGETingTargets)
            {
                weakSelf.isGETingTargets = YES;
                
                ARAppDelegate *appDelegate = (ARAppDelegate *)[UIApplication sharedApplication].delegate;
                NSString *targetURLString = [NSString stringWithFormat:@"http://%@/php/getButtonLayout.php?object=%@&state=%@", appDelegate.serverDomain, self.interface.name, self.stateIdString];
                NSURL *targetURL = [NSURL URLWithString:targetURLString];
                
                [[[NSURLSession sharedSession] dataTaskWithURL:targetURL completionHandler:^(NSData * _Nullable data, NSURLResponse * _Nullable response, NSError * _Nullable error) {
                    CGSize normalizedCollectionViewContentSize = CGSizeZero;
                    NSArray *targets = nil;
                    
                    // Create a new target array if possible.
                    if (data && isOKHTTPURLResponse(response))
                    {
                        NSArray *targetJSONArray = [NSJSONSerialization JSONObjectWithData:data options:kNilOptions error:nil];
                        BOOL isUsefulTargetJSONArray = targetJSONArray && (targetJSONArray.count > 1);
                        if (isUsefulTargetJSONArray)
                        {
                            normalizedCollectionViewContentSize = normalizedContentSizeForContentSizeArray(targetJSONArray[0]);
                            
                            NSArray *targetArrays = [targetJSONArray subarrayWithRange:NSMakeRange(1, targetJSONArray.count - 1)];
                            targets = targetsForTargetArrays(targetArrays);
                        }
                    }
                    
                    BOOL isNormalizedCollectionViewContentSize = !CGSizeEqualToSize(normalizedCollectionViewContentSize, CGSizeZero);
                    BOOL isNewTargets = (targets && ![targets isEqualToArray:weakSelf.targets]);
                    if (isNormalizedCollectionViewContentSize && isNewTargets)
                    {
                        // Update the user interface on the main thread.
                        dispatch_async(dispatch_get_main_queue(), ^{
                            weakSelf.normalizedCollectionViewContentSize = normalizedCollectionViewContentSize;
                            weakSelf.targets = targets;
                            [weakSelf.targetCollectionView reloadData];
                        });
                    }
                    
                    weakSelf.isGETingTargets = NO;
                }] resume];
            }
        });
        dispatch_source_set_timer(refreshTargetTableDispatchSource, DISPATCH_TIME_NOW, 1.0 * NSEC_PER_SEC, 0.1 * NSEC_PER_SEC);
        _refreshTargetTableDispatchSource = refreshTargetTableDispatchSource;
    }
    return _refreshTargetTableDispatchSource;
}

- (UIAlertController *)interfaceNameAlertControllerWithImageData:(NSData *)imageData
{
    UIAlertController *alertController = [UIAlertController alertControllerWithTitle:@"New Appliance"
                                                                             message:nil
                                                                      preferredStyle:UIAlertControllerStyleAlert];
    
    // 'Cancel' alert action.
    UIAlertAction *alertCancelAction = [UIAlertAction actionWithTitle:@"Cancel" style:UIAlertActionStyleCancel
                                                              handler:NULL];
    [alertController addAction:alertCancelAction];
    
    ARInterfaceFeedbackViewController * __weak weakSelf = self;
    // 'OK' alert action.
    UIAlertAction *alertOKAction = [UIAlertAction actionWithTitle:@"OK" style:UIAlertActionStyleDefault handler:^(UIAlertAction * _Nonnull action) {
        NSString *interfaceName = alertController.textFields.firstObject.text;
        [weakSelf uploadOriginalWithInterfaceName:interfaceName imageData:imageData];
    }];
    [alertController addAction:alertOKAction];
    
    // 'Name' text field.
    [alertController addTextFieldWithConfigurationHandler:^(UITextField * _Nonnull textField) {
        textField.placeholder = @"Name";
    }];
    
    return alertController;
}

- (NSString *)selectedTarget
{
    NSIndexPath *indexPath = [[self.targetCollectionView indexPathsForSelectedItems] firstObject];
    return (indexPath) ? ((ARTarget *)self.targets[indexPath.row]).name : nil;
}

- (void)uploadVideoWithImageData:(NSData *)imageData
{
    static NSTimeInterval lastUploadImageDataTime = 0;
    
    NSTimeInterval currentUploadImageDataTime = [[NSDate date] timeIntervalSince1970];
    ARAppDelegate *appDelegate = (ARAppDelegate *)[UIApplication sharedApplication].delegate;
    BOOL isUploadImageDataRateRestrictionExpired = (currentUploadImageDataTime - lastUploadImageDataTime) > appDelegate.uploadDelay;
    if (imageData && isUploadImageDataRateRestrictionExpired && !self.isPOSTingVideo)
    {
        self.isPOSTingVideo = YES;
        
        lastUploadImageDataTime = currentUploadImageDataTime;
        
        NSString *selectedTarget = [self selectedTarget];
        BOOL isGuidenceMode = (selectedTarget != nil);
        // Create and configure a new url request.
        NSMutableURLRequest *urlRequest = [[NSMutableURLRequest alloc] init];
        urlRequest.HTTPMethod = @"POST";
        // URL.
        NSString *interfaceUploadURLString = [NSString stringWithFormat:@"http://%@/php/uploadVideo.php", appDelegate.serverDomain];
        urlRequest.URL = [NSURL URLWithString:interfaceUploadURLString];
        // HTTP header.
        NSString *httpHeaderContentTypeString = [NSString stringWithFormat:@"multipart/form-data; boundary=%@", ARHeaderBoundaryString];
        [urlRequest setValue:httpHeaderContentTypeString forHTTPHeaderField: @"Content-Type"];
        // HTTP body.
        NSMutableData *httpBody = [NSMutableData data];
        [httpBody appendData:[[NSString stringWithFormat:@"--%@\r\n", ARHeaderBoundaryString] dataUsingEncoding:NSUTF8StringEncoding]];
        [httpBody appendData:[@"Content-Disposition: form-data; name=\"userid\"\r\n\r\n" dataUsingEncoding:NSUTF8StringEncoding]];
        [httpBody appendData:[[NSString stringWithFormat:@"%@\r\n", appDelegate.userID] dataUsingEncoding:NSUTF8StringEncoding]];
        [httpBody appendData:[[NSString stringWithFormat:@"--%@\r\n", ARHeaderBoundaryString] dataUsingEncoding:NSUTF8StringEncoding]];
        [httpBody appendData:[@"Content-Disposition: form-data; name=\"session\"\r\n\r\n" dataUsingEncoding:NSUTF8StringEncoding]];
        [httpBody appendData:[[NSString stringWithFormat:@"%@\r\n", self.interface.name] dataUsingEncoding:NSUTF8StringEncoding]];
        [httpBody appendData:[[NSString stringWithFormat:@"--%@\r\n", ARHeaderBoundaryString] dataUsingEncoding:NSUTF8StringEncoding]];
        [httpBody appendData:[@"Content-Disposition: form-data; name=\"mode\"\r\n\r\n" dataUsingEncoding:NSUTF8StringEncoding]];
        [httpBody appendData:[[NSString stringWithFormat:@"%@\r\n", (isGuidenceMode) ? @"2" : @"1"] dataUsingEncoding:NSUTF8StringEncoding]];
        [httpBody appendData:[[NSString stringWithFormat:@"--%@\r\n", ARHeaderBoundaryString] dataUsingEncoding:NSUTF8StringEncoding]];
        [httpBody appendData:[@"Content-Disposition: form-data; name=\"target\"\r\n\r\n" dataUsingEncoding:NSUTF8StringEncoding]];
        [httpBody appendData:[[NSString stringWithFormat:@"%@\r\n", selectedTarget] dataUsingEncoding:NSUTF8StringEncoding]];
        [httpBody appendData:[[NSString stringWithFormat:@"--%@\r\n", ARHeaderBoundaryString] dataUsingEncoding:NSUTF8StringEncoding]];
        [httpBody appendData:[[NSString stringWithFormat:@"Content-Disposition: form-data; name=\"pictures[0]\"; filename=\"%@.jpg\"\r\n", filenameWithPrefix(@"VIDEO", self.interface.name)] dataUsingEncoding:NSUTF8StringEncoding]];
        [httpBody appendData:[@"Content-Type:image/jpeg\r\n\r\n" dataUsingEncoding:NSUTF8StringEncoding]];
        [httpBody appendData:imageData];
        [httpBody appendData:[@"\r\n" dataUsingEncoding:NSUTF8StringEncoding]];
        [httpBody appendData:[[NSString stringWithFormat:@"--%@--\r\n", ARHeaderBoundaryString] dataUsingEncoding:NSUTF8StringEncoding]];
        urlRequest.HTTPBody = httpBody;
        
        ARInterfaceFeedbackViewController * __weak weakSelf = self;
        // Upload interface image.
        [[[NSURLSession sharedSession] dataTaskWithRequest:urlRequest completionHandler:^(NSData * _Nullable data, NSURLResponse * _Nullable response, NSError * _Nullable error) {
            weakSelf.isPOSTingVideo = NO;
        }] resume];
    }
}

- (void)uploadOriginalWithInterfaceName:(NSString *)interfaceName imageData:(NSData *)imageData
{
    // Create and configure a new url request.
    NSMutableURLRequest *urlRequest = [[NSMutableURLRequest alloc] init];
    urlRequest.HTTPMethod = @"POST";
    // URL.
    ARAppDelegate *appDelegate = (ARAppDelegate *)[UIApplication sharedApplication].delegate;
    NSString *interfaceUploadURLString = [NSString stringWithFormat:@"http://%@/php/uploadOriginal.php", appDelegate.serverDomain];
    urlRequest.URL = [NSURL URLWithString:interfaceUploadURLString];
    // HTTP header.
    NSString *httpHeaderContentTypeString = [NSString stringWithFormat:@"multipart/form-data; boundary=%@", ARHeaderBoundaryString];
    [urlRequest setValue:httpHeaderContentTypeString forHTTPHeaderField: @"Content-Type"];
    // HTTP body.
    NSMutableData *httpBody = [NSMutableData data];
    [httpBody appendData:[[NSString stringWithFormat:@"--%@\r\n", ARHeaderBoundaryString] dataUsingEncoding:NSUTF8StringEncoding]];
    [httpBody appendData:[@"Content-Disposition: form-data; name=\"userid\"\r\n\r\n" dataUsingEncoding:NSUTF8StringEncoding]];
    [httpBody appendData:[[NSString stringWithFormat:@"%@\r\n", appDelegate.userID] dataUsingEncoding:NSUTF8StringEncoding]];
    [httpBody appendData:[[NSString stringWithFormat:@"--%@\r\n", ARHeaderBoundaryString] dataUsingEncoding:NSUTF8StringEncoding]];
    [httpBody appendData:[@"Content-Disposition: form-data; name=\"session\"\r\n\r\n" dataUsingEncoding:NSUTF8StringEncoding]];
    [httpBody appendData:[[NSString stringWithFormat:@"%@\r\n", interfaceName] dataUsingEncoding:NSUTF8StringEncoding]];
    [httpBody appendData:[[NSString stringWithFormat:@"--%@\r\n", ARHeaderBoundaryString] dataUsingEncoding:NSUTF8StringEncoding]];
    [httpBody appendData:[[NSString stringWithFormat:@"Content-Disposition: form-data; name=\"pictures[0]\"; filename=\"%@.jpg\"\r\n", filenameWithPrefix(@"ORIGINAL", interfaceName)] dataUsingEncoding:NSUTF8StringEncoding]];
    [httpBody appendData:[@"Content-Type:image/jpeg\r\n\r\n" dataUsingEncoding:NSUTF8StringEncoding]];
    [httpBody appendData:imageData];
    [httpBody appendData:[@"\r\n" dataUsingEncoding:NSUTF8StringEncoding]];
    [httpBody appendData:[[NSString stringWithFormat:@"--%@--\r\n", ARHeaderBoundaryString] dataUsingEncoding:NSUTF8StringEncoding]];
    urlRequest.HTTPBody = httpBody;
    
    ARInterfaceFeedbackViewController * __weak weakSelf = self;
    // Upload interface.
    [[[NSURLSession sharedSession] dataTaskWithRequest:urlRequest completionHandler:^(NSData * _Nullable data, NSURLResponse * _Nullable response, NSError * _Nullable error) {
        // Exit triggers user interface updates
        // that need to happen on the main thread.
        dispatch_async(dispatch_get_main_queue(), ^{
            [weakSelf exit:weakSelf.exitButton];
        });
    }] resume];
}


#pragma mark - Feedback

- (void)didFinishFeedback:(NSNotification *)notification
{
    NSString *politeFeedback = self.politeFeedback;
    BOOL feedbackWasSuccessful = [[[notification userInfo] objectForKey:UIAccessibilityAnnouncementKeyWasSuccessful] boolValue];
    if (politeFeedback.length && feedbackWasSuccessful)
    {
        self.feedbackLabel.text = politeFeedback;
        
        ARAppDelegate *appDelegate = (ARAppDelegate *)[UIApplication sharedApplication].delegate;
        if (appDelegate.sound && isNoFingerFeedback(politeFeedback))
        {
            AudioServicesPlaySystemSound(self.noFingerSoundID);
        }
        else if (appDelegate.sound && isNoObjectFeedback(politeFeedback))
        {
            AudioServicesPlaySystemSound(self.noObjectSoundID);
        }
        else
        {
            UIAccessibilityPostNotification(UIAccessibilityAnnouncementNotification, politeFeedback);
        }
    }
    self.politeFeedback = nil;
}


#pragma mark - User Interface

- (IBAction)capture:(UIButton *)captureButton
{
    AVCaptureStillImageOutput *captureStillImageOutput = self.captureStillImageOutput;
    AVCaptureConnection *captureConnection = [captureStillImageOutput connectionWithMediaType:AVMediaTypeVideo];
    
    // Update orientation before still image capture.
    captureConnection.videoOrientation = videoOrientationForDeviceOrientation([[UIDevice currentDevice] orientation]);
    
    ARInterfaceFeedbackViewController * __weak weakSelf = self;
    [captureStillImageOutput captureStillImageAsynchronouslyFromConnection:captureConnection completionHandler:^(CMSampleBufferRef imageDataSampleBuffer, NSError *error) {
        if (imageDataSampleBuffer)
        {
            NSData *imageData = [AVCaptureStillImageOutput jpegStillImageNSDataRepresentation:imageDataSampleBuffer];
            
            // Makes user interface changes that
            // need to happen on the main thread.
            dispatch_async(dispatch_get_main_queue(), ^{
                if (isRecaptureMode(weakSelf.mode))
                {
                    [weakSelf uploadOriginalWithInterfaceName:weakSelf.interface.name imageData:imageData];
                    [weakSelf exit:weakSelf.exitButton];
                }
                else
                {
                    UIAlertController *alertController = [weakSelf interfaceNameAlertControllerWithImageData:imageData];
                    [weakSelf presentViewController:alertController animated:YES completion:NULL];
                }
            });
        }
    }];
}

- (IBAction)exit:(UIButton *)exitButton
{
    ARInterfaceFeedbackViewController * __weak weakSelf = self;
    [self.presentingViewController dismissViewControllerAnimated:YES completion:^{
        // Only set properties on the main thread.
        dispatch_async(dispatch_get_main_queue(), ^{
            weakSelf.interface = nil;
            weakSelf.mode = ARInterfaceFeedbackViewControllerUndefinedMode;
        });
    }];
}


#pragma mark - Capture Video Data Output Sample Buffer Delegate

- (void)captureOutput:(AVCaptureOutput *)captureOutput didOutputSampleBuffer:(CMSampleBufferRef)sampleBuffer fromConnection:(AVCaptureConnection *)connection
{
    if (sampleBuffer && isFeedbackMode(self.mode) && !self.isPOSTingVideo)
    {
        connection.videoOrientation = videoOrientationForDeviceOrientation([[UIDevice currentDevice] orientation]);
        // Get the image buffer.
        CVImageBufferRef imageBuffer = CMSampleBufferGetImageBuffer(sampleBuffer);
        // Lock the image buffer.
        CVPixelBufferLockBaseAddress(imageBuffer, 0);
        void *imageBufferBaseAddress = CVPixelBufferGetBaseAddress(imageBuffer);
        // Get the image buffer details.
        size_t imageBufferBytesPerRow = CVPixelBufferGetBytesPerRow(imageBuffer);
        size_t imageBufferWidth = CVPixelBufferGetWidth(imageBuffer);
        size_t imageBufferHeight = CVPixelBufferGetHeight(imageBuffer);
        // Create a new quartz interface image from the image buffer and the image buffer details.
        CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
        CGContextRef context = CGBitmapContextCreate(imageBufferBaseAddress, imageBufferWidth, imageBufferHeight, 8, imageBufferBytesPerRow, colorSpace, kCGBitmapByteOrder32Little | kCGImageAlphaPremultipliedFirst);
        CGImageRef quartzImage = CGBitmapContextCreateImage(context);
        CGContextRelease(context);
        CGColorSpaceRelease(colorSpace);
        // Unlock the image buffer.
        CVPixelBufferUnlockBaseAddress(imageBuffer, 0);
        // Create a new interface image from the quartz image.
        UIImage *image = [UIImage imageWithCGImage:quartzImage];
        CGImageRelease(quartzImage);
        
        ARAppDelegate *appDelegate = (ARAppDelegate *)[UIApplication sharedApplication].delegate;
        NSData *imageData = UIImageJPEGRepresentation(image, appDelegate.imageCompression);
        
        ARInterfaceFeedbackViewController * __weak weakSelf = self;
        // Uploading an original image may trigger user interface
        // updates that need to happen on the main thread.
        dispatch_async(dispatch_get_main_queue(), ^{
            [weakSelf uploadVideoWithImageData:imageData];
        });
    }
}


#pragma mark - Collection View Data Source

- (NSInteger)numberOfSectionsInCollectionView:(UICollectionView *)collectionView
{
    return 1;
}

- (NSInteger)collectionView:(UICollectionView *)collectionView numberOfItemsInSection:(NSInteger)section
{
    return self.targets.count;
}

- (UICollectionViewCell *)collectionView:(UICollectionView *)collectionView cellForItemAtIndexPath:(NSIndexPath *)indexPath
{
    ARTargetCollectionViewCell *targetCollectionViewCell = [collectionView dequeueReusableCellWithReuseIdentifier:ARTargetCollectionViewCellReuseIdentifier forIndexPath:indexPath];
    targetCollectionViewCell.textLabel.text = [(ARTarget *)self.targets[indexPath.row] name];
    return targetCollectionViewCell;
}


#pragma mark - Collection View Delegate

- (BOOL)collectionView:(UICollectionView *)collectionView shouldSelectItemAtIndexPath:(NSIndexPath *)indexPath
{
    BOOL willSelectSelectedRow = [indexPath isEqual:[[collectionView indexPathsForSelectedItems] firstObject]];
    if (willSelectSelectedRow)
    {
        [collectionView deselectItemAtIndexPath:indexPath animated:YES];
    }
    return !willSelectSelectedRow;
}


#pragma mark - Collection View Interface Layout Data Source

- (CGSize)normalizedContentSizeForCollectionView:(nullable UICollectionView *)collectionView
{
    return self.normalizedCollectionViewContentSize;
}

- (nullable NSArray *)targetsForCollectionView:(nullable UICollectionView *)collectionView
{
    return [self targets];
}

@end

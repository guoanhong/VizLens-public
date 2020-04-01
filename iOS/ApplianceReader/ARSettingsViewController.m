//
//  ARSettingsViewController.m
//  ApplianceReader
//

#import "ARSettingsViewController.h"

@import AVFoundation;

#import "ARAppDelegate.h"


#pragma mark - Helpers

NS_INLINE NSString *imageSizeForSelectedSegmentIndex(NSInteger selectedSegmentIndex)
{
    switch (selectedSegmentIndex)
    {
        case 0:
            return AVCaptureSessionPreset640x480;
        default:
        case 1:
            return AVCaptureSessionPreset1280x720;
        case 2:
            return AVCaptureSessionPreset1920x1080;
    }
}

NS_INLINE NSInteger selectedSegmentIndexForImageSize(NSString *imageSize)
{
    if ([imageSize isEqualToString:AVCaptureSessionPreset640x480])
    {
        return 0;
    }
    else if ([imageSize isEqualToString:AVCaptureSessionPreset1920x1080])
    {
        return 2;
    }
    else
    {
        return 1;
    }
}


#pragma mark - Private Interface

@interface ARSettingsViewController () <UITextFieldDelegate>

// User interface.
@property (nonatomic, weak) IBOutlet UIButton *cancelButton;
@property (nonatomic, weak) IBOutlet UILabel *feedbackDelayLabel;
@property (nonatomic, weak) IBOutlet UISlider *feedbackDelaySlider;
@property (nonatomic, weak) IBOutlet UILabel *imageCompressionLabel;
@property (nonatomic, weak) IBOutlet UISlider *imageCompressionSlider;
@property (nonatomic, weak) IBOutlet UISegmentedControl *imageSizeSegmentedControl;
@property (nonatomic, weak) IBOutlet UISwitch *politeSwitch;
@property (nonatomic, weak) IBOutlet UITextField *serverDomainTextField;
@property (nonatomic, weak) IBOutlet UISwitch *soundSwitch;
@property (nonatomic, weak) IBOutlet UILabel *uploadDelayLabel;
@property (nonatomic, weak) IBOutlet UISlider *uploadDelaySlider;
@property (nonatomic, weak) IBOutlet UITextField *userIDTextField;

- (IBAction)cancel:(UIButton *)cancelButton;
- (IBAction)save:(UIButton *)saveButton;

- (IBAction)feedbackDelaySliderValueChanged:(UISlider *)feedbackDelaySlider;
- (IBAction)imageCompressionSliderValueChanged:(UISlider *)imageCompressionSlider;
- (IBAction)uploadDelaySliderValueChanged:(UISlider *)uploadDelaySlider;

@end


#pragma mark - Implementation

@implementation ARSettingsViewController

- (BOOL)prefersStatusBarHidden
{
    return YES;
}

- (void)viewWillAppear:(BOOL)animated
{
    [super viewWillAppear:animated];
    
    // Load settings.
    ARAppDelegate *appDelegate = (ARAppDelegate *)[UIApplication sharedApplication].delegate;
    float feedbackDelay = appDelegate.feedbackDelay;
    self.feedbackDelayLabel.text = [NSString stringWithFormat:@"%.1f", feedbackDelay];
    self.feedbackDelaySlider.value = feedbackDelay;
    float imageCompression = appDelegate.imageCompression;
    self.imageCompressionLabel.text = [NSString stringWithFormat:@"%.1f", imageCompression];
    self.imageCompressionSlider.value = imageCompression;
    self.imageSizeSegmentedControl.selectedSegmentIndex = selectedSegmentIndexForImageSize(appDelegate.imageSize);
    self.politeSwitch.on = appDelegate.polite;
    self.serverDomainTextField.text = appDelegate.serverDomain;
    self.soundSwitch.on = appDelegate.sound;
    float uploadDelay = appDelegate.uploadDelay;
    self.uploadDelayLabel.text = [NSString stringWithFormat:@"%.1f", uploadDelay];
    self.uploadDelaySlider.value = uploadDelay;
    self.userIDTextField.text = appDelegate.userID;
}


#pragma mark - Text Field Delegate

- (BOOL)textFieldShouldReturn:(UITextField *)textField
{
    [textField resignFirstResponder];
    return YES;
}


#pragma mark - User Interface

- (IBAction)cancel:(UIButton *)cancelButton
{
    [self.presentingViewController dismissViewControllerAnimated:YES completion:NULL];
}

- (IBAction)save:(UIButton *)saveButton
{
    // Save settings.
    ARAppDelegate *appDelegate = (ARAppDelegate *)[UIApplication sharedApplication].delegate;
    appDelegate.feedbackDelay = self.feedbackDelaySlider.value;
    appDelegate.imageCompression = self.imageCompressionSlider.value;
    appDelegate.imageSize = imageSizeForSelectedSegmentIndex(self.imageSizeSegmentedControl.selectedSegmentIndex);
    appDelegate.polite = self.politeSwitch.isOn;
    appDelegate.serverDomain = self.serverDomainTextField.text;
    appDelegate.sound = self.soundSwitch.isOn;
    appDelegate.uploadDelay = self.uploadDelaySlider.value;
    appDelegate.userID = self.userIDTextField.text;
    
    [self cancel:self.cancelButton];
}

- (IBAction)feedbackDelaySliderValueChanged:(UISlider *)feedbackDelaySlider
{
    feedbackDelaySlider.value = truncf(feedbackDelaySlider.value * 10.0) / 10.0;
    self.feedbackDelayLabel.text = [NSString stringWithFormat:@"%.1f", feedbackDelaySlider.value];
}

- (IBAction)imageCompressionSliderValueChanged:(UISlider *)imageCompressionSlider
{
    imageCompressionSlider.value = truncf(imageCompressionSlider.value * 10.0) / 10.0;
    self.imageCompressionLabel.text = [NSString stringWithFormat:@"%.1f", imageCompressionSlider.value];
}

- (IBAction)uploadDelaySliderValueChanged:(UISlider *)uploadDelaySlider
{
    uploadDelaySlider.value = truncf(uploadDelaySlider.value * 10.0) / 10.0;
    self.uploadDelayLabel.text = [NSString stringWithFormat:@"%.1f", uploadDelaySlider.value];
}

@end

//
//  ARDevViewController.m
//  ApplianceReader
//

#import "ARDevViewController.h"

#import <opencv2/calib3d.hpp>
#import <opencv2/videoio/cap_ios.h>
#import <opencv2/imgcodecs/ios.h>
#import <opencv2/video.hpp>
#import <opencv2/xfeatures2d.hpp>

using namespace cv;
using namespace cv::xfeatures2d;


#pragma mark - Helpers

NS_INLINE void ARGoodHomography(std::vector<KeyPoint> queryKeypoints,
                                std::vector<KeyPoint> trainKeypoints,
                                std::vector<DMatch> matches,
                                Mat &homography)
{
    if (!queryKeypoints.size() ||
        !trainKeypoints.size() ||
        !matches.size()) return;
    
    std::vector<Point2f> sourcePoints;
    std::vector<Point2f> destinationPoints;
    for (size_t idx = 0; idx < matches.size(); idx++)
    {
        // Find points associated with matched descriptors.
        sourcePoints.push_back(trainKeypoints[matches[idx].trainIdx].pt);
        destinationPoints.push_back(queryKeypoints[matches[idx].queryIdx].pt);
    }
    homography = findHomography(sourcePoints, destinationPoints, RANSAC);
    // FIXME: Reject unreasonable homography.
}

NS_INLINE void ARGoodMatches(std::vector<DMatch> matches,
                             std::vector<DMatch> &goodMatches)
{
    if (!matches.size()) return;
    
    double maxDistance = 0.;
    double minDistance = DBL_MAX;
    // Compute max and min distances between keypoints.
    for (size_t idx = 0; idx < matches.size(); idx++)
    {
        double distance = matches[idx].distance;
        if (distance > maxDistance) maxDistance = distance;
        if (distance < minDistance) minDistance = distance;
    }
    // Find good matches (i.e. distance < 3 * minDistance).
    for (size_t idx = 0; idx < matches.size(); idx++)
    {
        DMatch match = matches[idx];
        if (match.distance < 3 * minDistance) goodMatches.push_back(match);
    }
}

NS_INLINE void ARMaxAreaFingerHull(Mat queryImageBinary,
                                   std::vector<cv::Point> &maxAreaFingerHull)
{
    // Find contours.
    std::vector<std::vector<cv::Point> > contours;
    findContours(queryImageBinary, contours, RETR_TREE, CHAIN_APPROX_SIMPLE);
    
    if (!contours.size()) return;
    
    // Find the finger hull with the largest area.
    std::vector<cv::Point> largestContour = contours[0];
    for (size_t idx = 1; idx < contours.size(); idx++)
    {
        std::vector<cv::Point> contour = contours[idx];
        if (contourArea(largestContour) < contourArea(contour))
            largestContour = contour;
    }
    cv::Rect contourRect(boundingRect(largestContour));
    cv::Rect queryImageRect(0, 0, queryImageBinary.cols, queryImageBinary.rows);
    // Reject contours islands (i.e. finger must be connected to frame).
    if (contourRect.tl().x == queryImageRect.tl().x ||
        contourRect.tl().y == queryImageRect.tl().y ||
        contourRect.br().x == queryImageRect.br().x ||
        contourRect.br().y == queryImageRect.br().y)
        convexHull(largestContour, maxAreaFingerHull);
}

NS_INLINE bool ARPointIsZero(cv::Point2f point)
{
    return (point.x == 0.) && (point.y == 0.);
}

NS_INLINE void ARSkinImageBinary(Mat queryImageBGRA, Mat trainImageBGRA,
                                 Mat &skinImageBinary)
{
    if (queryImageBGRA.empty() || trainImageBGRA.empty()) return;
    
    // Adjust query image colors to better match train image colors.
    queryImageBGRA -= mean(queryImageBGRA) - mean(trainImageBGRA);
    
    // Filter out non-skin colors.
    Mat queryImageBinary(queryImageBGRA.size(), CV_8UC1);
    Mat queryImageHSV(queryImageBGRA.size(), CV_64FC3);
    cvtColor(queryImageBGRA, queryImageHSV, CV_BGRA2BGR);
    cvtColor(queryImageHSV, queryImageHSV, CV_BGR2HSV);
    inRange(queryImageHSV, cv::Scalar(0, 90, 60), cv::Scalar(20, 150, 255),
            skinImageBinary);
    
    // Blur and dilate to cleanup.
    medianBlur(skinImageBinary, skinImageBinary, 7);
    Mat dilateKernel = getStructuringElement(MORPH_RECT, cv::Size(5, 5));
    morphologyEx(skinImageBinary, skinImageBinary, MORPH_DILATE, dilateKernel);
}

NS_INLINE void ARFingerTip(std::vector<cv::Point> fingerHull,
                           cv::Point2f &fingerTip, double &radius)
{
    if (!fingerHull.size()) return;
    
    // Find the finger hull point with the smallest y value (i.e. the
    // highest on the screen) and both of its neighboring points.
    size_t minYIdx = 0;
    for (size_t idx = 1; idx < fingerHull.size(); idx++)
    {
        if (fingerHull[idx].y < fingerHull[minYIdx].y) minYIdx = idx;
    }
    // Wrap *Idx around if necessary.
    size_t endIdx = (minYIdx == fingerHull.size() - 1) ? 0 : minYIdx + 1;
    size_t startIdx = (minYIdx == 0) ? fingerHull.size() - 1 : minYIdx - 1;
    
    // Find finger tip.
    cv::Point endPoint = fingerHull[endIdx];
    cv::Point startPoint = fingerHull[startIdx];
    cv::Point minYPoint = fingerHull[minYIdx];
    double endDistance = norm(endPoint - minYPoint);
    double startDistance = norm(startPoint - minYPoint);
    double meanDistance = (endDistance + startDistance) / 2;
    // Search for the END of the finger tip (i.e. check hull points AFTER
    // minYPoint until a sufficently long side is found.
    double nextDistance;
    size_t nextIdx;
    cv::Point nextPoint;
    for (nextIdx = endIdx, nextPoint = endPoint,
         nextDistance = endDistance; (nextPoint != minYPoint) &&
         (nextDistance < 2 * meanDistance) &&
         (endDistance < 6 * meanDistance);
         nextIdx = (nextIdx == fingerHull.size() - 1) ? 0 : nextIdx + 1,
         nextPoint = fingerHull[nextIdx],
         nextDistance = norm(nextPoint - endPoint))
    {
        endIdx = nextIdx;
        endPoint = nextPoint;
        endDistance += nextDistance;
    }
    // Search for the START of the finger tip (i.e. check hull points BEFORE
    // minYPoint until a sufficently long side is found.
    for (nextIdx = startIdx, nextPoint = startPoint,
         nextDistance = startDistance; (nextPoint != minYPoint) &&
         (nextDistance < 2 * meanDistance) &&
         (startDistance < 6 * meanDistance);
         nextIdx = (nextIdx == 0) ? fingerHull.size() - 1 : nextIdx - 1,
         nextPoint = fingerHull[nextIdx],
         nextDistance = norm(nextPoint - startPoint))
    {
        startIdx = nextIdx;
        startPoint = nextPoint;
        startDistance += nextDistance;
    }
    // Finger tip centroid.
    fingerTip.x = (endPoint.x + startPoint.x + minYPoint.x) / 3.;
    fingerTip.y = (endPoint.y + startPoint.y + minYPoint.y) / 3.;
    // Radius.
    radius = norm(endPoint - startPoint) / 2.;
}


#pragma mark - Private Interface

@interface ARDevViewController () <CvVideoCameraDelegate>

@property (nonatomic, weak) IBOutlet UIImageView *imageView;
@property (nonatomic) CvVideoCamera *videoCamera;

@end


#pragma mark - Implementation

@implementation ARDevViewController

- (void)viewDidAppear:(BOOL)animated
{
    [super viewDidAppear:animated];
    
    [self.videoCamera start];
}

- (void)viewDidLoad
{
    [super viewDidLoad];
    
    self.videoCamera = [[CvVideoCamera alloc] initWithParentView:self.imageView];
    self.videoCamera.defaultAVCaptureDevicePosition = AVCaptureDevicePositionBack;
    self.videoCamera.defaultAVCaptureSessionPreset = AVCaptureSessionPresetiFrame960x540;
    self.videoCamera.defaultAVCaptureVideoOrientation = AVCaptureVideoOrientationPortrait;
    self.videoCamera.defaultFPS = 30;
    self.videoCamera.delegate = self;
}

- (void)processImage:(Mat&)rawImageBGRA
{
    // FPS start.
    NSDate *processImageDate = [NSDate date];
    
    static Ptr<SURF> detector;
    // Reuse detector.
    if (detector.empty()) detector = SURF::create(400);
    
    // Get interface image keypoints and descriptors.
    static Mat interfaceImageBGRA;
    static Mat interfaceDescriptors;
    static std::vector<KeyPoint> interfaceKeypoints;
    // Reuse interface image, interface keypoints and interface descriptors.
    if (interfaceImageBGRA.empty())
    {
        interfaceImageBGRA = Mat(cv::Size(270, 480), CV_8UC4);
        Mat interfaceImageGRAY(cv::Size(270, 480), CV_8UC1);
        // UIImageToMat loads channels in RGBA order.
        UIImageToMat([UIImage imageNamed:@"breadman_plus_270x480"],
                     interfaceImageBGRA);
        cvtColor(interfaceImageBGRA, interfaceImageBGRA, CV_RGBA2BGRA);
        cvtColor(interfaceImageBGRA, interfaceImageGRAY, CV_BGRA2GRAY);
        detector->detectAndCompute(interfaceImageGRAY, Mat(), interfaceKeypoints,
                                   interfaceDescriptors);
    }
    
    // Get scene image keypoints and descriptors.
    Mat sceneImageBGRA(cv::Size(432, 768), CV_8UC4);
    Mat sceneImageGRAY(cv::Size(432, 768), CV_8UC1);
    // Scene image has the edges from raw image removed to increase
    // speed and help prevent detecting out of frame interfaces.
    rawImageBGRA(cv::Rect(54, 96, 432, 768)).copyTo(sceneImageBGRA);
    cvtColor(sceneImageBGRA, sceneImageGRAY, CV_BGRA2GRAY);
    Mat sceneDescriptors;
    std::vector<KeyPoint> sceneKeypoints;
    detector->detectAndCompute(sceneImageGRAY, Mat(), sceneKeypoints,
                               sceneDescriptors);
    
    if (interfaceDescriptors.empty() || sceneDescriptors.empty()) return;
    
    Ptr<FlannBasedMatcher> matcher;
    // Reuse matcher.
    if (matcher.empty()) matcher = FlannBasedMatcher::create();
    
    // Match interface and scene descriptors.
    std::vector<DMatch> goodMatches;
    std::vector<DMatch> matches;
    matcher->match(sceneDescriptors, interfaceDescriptors, matches);
    ARGoodMatches(matches, goodMatches);
    
    // Get scene image homography.
    Mat sceneHomography;
    ARGoodHomography(sceneKeypoints, interfaceKeypoints, goodMatches,
                     sceneHomography);
    
    if (sceneHomography.empty()) return;
    
    // Get raw image homography.
    Mat rawHomography = Mat::eye(cv::Size(3, 3), CV_64FC1);
    // Apply a translation to account for the size difference
    // between the scene image and the raw image.
    rawHomography.at<double>(0, 2) = 54.;
    rawHomography.at<double>(1, 2) = 96.;
    rawHomography *= sceneHomography;
    
    // Get scene interface image (i.e. extract the interface
    // image from the scene image) for processing.
    Mat sceneInterfaceImageBGRA(interfaceImageBGRA.size(), CV_8UC4);
    warpPerspective(rawImageBGRA, sceneInterfaceImageBGRA, rawHomography.inv(),
                    sceneInterfaceImageBGRA.size());
    
    // Get binary skin image.
    Mat skinImageBinary(interfaceImageBGRA.size(), CV_8UC1);
    ARSkinImageBinary(sceneInterfaceImageBGRA, interfaceImageBGRA,
                      skinImageBinary);
    
    // Get finger hull.
    std::vector<cv::Point> maxAreaFingerHull;
    ARMaxAreaFingerHull(skinImageBinary, maxAreaFingerHull);
    
    // Get finger tip.
    cv::Point2f fingerTip;
    double radius;
    ARFingerTip(maxAreaFingerHull, fingerTip, radius);
    
    
#pragma mark - Drawing
    
    
    // Translate interesting points for drawing.
    std::vector<Point2f> scenePoints(5);
    // Translate interface image corners.
    scenePoints[0] = cv::Point2f(0., 0.);
    scenePoints[1] = cv::Point2f(interfaceImageBGRA.cols, 0.);
    scenePoints[2] = cv::Point2f(interfaceImageBGRA.cols,
                                 interfaceImageBGRA.rows);
    scenePoints[3] = cv::Point2f(0., interfaceImageBGRA.rows);
    // Translate finger tip.
    scenePoints[4] = fingerTip;
    std::vector<Point2f> rawPoints(5);
    perspectiveTransform(scenePoints, rawPoints, rawHomography);
    
    Scalar blueColor = Scalar(210, 139, 38, 255);
    Scalar pinkColor = Scalar(130, 54, 211, 255);
    int thickness = 2;
    int fontFace = FONT_HERSHEY_SIMPLEX;
    // Connect all four interface image corners.
    line(rawImageBGRA, rawPoints[0], rawPoints[1], blueColor, thickness);
    line(rawImageBGRA, rawPoints[1], rawPoints[2], blueColor, thickness);
    line(rawImageBGRA, rawPoints[2], rawPoints[3], blueColor, thickness);
    line(rawImageBGRA, rawPoints[3], rawPoints[0], blueColor, thickness);
    // Circle finger tip if present.
    if (!ARPointIsZero(fingerTip))
    {
        // Quick high-pass filter.
        static double filteredRadius = 0.;
        filteredRadius = filteredRadius * .6 + radius * .4;
        
        circle(rawImageBGRA, rawPoints[4], filteredRadius, pinkColor, thickness);
    }
    // FPS end.
    double processImageFPS = fabs(1 / [processImageDate timeIntervalSinceNow]);
    NSString *string = [NSString stringWithFormat:@"fps:%.2f", processImageFPS];
    putText(rawImageBGRA, string.UTF8String,
            cv::Point(30, rawImageBGRA.rows - 30), fontFace, 1.6, pinkColor,
            thickness);
}

@end

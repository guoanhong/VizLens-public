/* Image Utils Header */

#include <iostream>
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/calib3d/calib3d.hpp"
#include "opencv2/imgproc/imgproc.hpp"

using namespace cv;
using namespace std;

double calculateLighting(Mat img);
void lightingDetection(Mat img);
void gpuFindGoodMatches(vector< DMatch > matches, vector< DMatch > &good_matches);
void findH(vector<KeyPoint> keypoints_object, vector<KeyPoint> keypoints_scene, vector<DMatch> good_matches, Mat &H, vector<uchar> &inliers);
void getImageCorners(Mat img_object, vector<Point2f> &object_corners);
bool findRect(vector<Point2f> scene_corners, Rect &rect);
void drawRects(Mat img_results, vector<Point2f> object_corners, vector<Point2f> scene_corners, Mat img_object);
void skinColorAdjustment(Mat img_object, Mat &img_skin);
void skinColorFilter(Mat img_skin, Mat &img_skin_binary);
void skinColorSubtractor(Mat img_skin, Mat img_object, Mat &img_skin_binary);
int angleBetween(Point tip, Point next, Point prev);
void findConvexHullBasedFingertipLocation(vector<vector<Point> > contours, vector<vector<Point> > hull, int index_of_biggest_contour, double avg_button_size, vector<Point2f> &fingertip_location, Mat &draw_contours);
void findConvexHullDefectBasedFingertipLocation(vector<vector<Point> > contours, vector<vector<Point> > hull, int index_of_biggest_contour, double avg_button_size, vector<Point2f> &fingertip_location, Mat &draw_contours);
void findFingertipLocation (Mat img_skin_binary, int area, Mat H, double avg_button_size, vector<Point2f> &fingertip_location);
bool checkNewImgFromPool(vector<Mat> &new_img_pool, vector<vector<KeyPoint>> &new_keypoints_pool, vector<Mat> &new_descriptors_pool, 
    Mat &img_scene, vector<KeyPoint> &keypoints_scene, Mat &descriptors_scene);
int getButtonIndex(Point2f fingertip_location, vector<vector<int> > button_map, vector<string> true_labels);
bool matchesCurrentState(Mat descriptors_scene, vector<KeyPoint> keypoints_scene, Mat curr_state_img_scene, Mat descriptors_curr_state, vector<KeyPoint> keypoints_curr_state, float benchmark_ratio, float match_expectation_ratio);
bool matchesExpectedState(Mat descriptors_scene, vector<KeyPoint> keypoints_scene, Mat descriptors_expected_state, vector<KeyPoint> keypoints_expected_state, float benchmark_ratio, float match_expectation_ratio);
bool isNewState(float ratio_max, float ratio_lower, float ratio_upper, int state_index, Mat img_scene, vector<string> ocr_object_vector, float ocr_threshold_upper, float ocr_threshold_lower);
// void detectScreen(Mat &img_scene, bool &is_cropped, bool &is_skipped);
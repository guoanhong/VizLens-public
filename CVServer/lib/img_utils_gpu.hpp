/* Image Utils Header */

#include <iostream>
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/calib3d/calib3d.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/nonfree/nonfree.hpp"
#include <opencv2/gpu/gpu.hpp>
#include "opencv2/nonfree/gpu.hpp"

using namespace cv;
using namespace std;

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
bool checkNewImgFromPool(vector<Mat> &new_img_pool, vector<GpuMat> &new_img_pool_gpu, vector<vector<KeyPoint>> &new_keypoints_pool, vector<GpuMat> &new_keypoints_pool_gpu, vector<Mat> &new_descriptors_pool, vector<GpuMat> &new_descriptors_pool_gpu, 
    GpuMat &img_scene_gpu, vector<KeyPoint> &keypoints_scene, GpuMat &descriptors_scene_gpu);
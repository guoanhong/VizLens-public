/* Image Utils Implementation */

#include "img_utils_gpu.hpp"

const int MIN_FINGER_DEPTH = 20;
const int MAX_FINGER_ANGLE = 60;
const double PI = 3.1415926;

extern string id;
extern string image;
extern string mode;
extern string target;
extern string state;
extern string feedback;
extern string guidance;
extern int idle;

void lightingDetection(Mat img) {
    double img_mean = sum(img)[0] / (img.cols * img.rows);
    
    if (img_mean < 20) {
        cout << "Too dark!" << img_mean << endl;
    } else if (img_mean > 150) {
        cout << "Too bright!" << img_mean << endl;
    } else {
        cout << "Good lighting!" << img_mean << endl;
    }
    
    // When object is 87, good. When scene is at 125, a bit too bright.
}

void gpuFindGoodMatches(vector< DMatch > matches, vector< DMatch > &good_matches) {
    //-- Quick calculation of max and min distances between keypoints
    double max_dist = 0; double min_dist = 100;
    for (int i = 0; i < matches.size(); i++) {
        double dist = matches[i].distance;
        if (dist < min_dist) min_dist = dist;
        if (dist > max_dist) max_dist = dist;
    }
    
    // ORIGINAL OUTPUT
    // printf("-- Max dist : %f \n", max_dist );
    // printf("-- Min dist : %f \n\n", min_dist );
    
    //-- Draw only "good" matches (i.e. whose distance is less than 3*min_dist )
    min_dist = max(min_dist, 0.05);
    for (int i = 0; i < matches.size(); i++) {
        if (matches[i].distance < 3 * min_dist) {
            good_matches.push_back( matches[i]);
        }
    }
}

void findH(vector<KeyPoint> keypoints_object, vector<KeyPoint> keypoints_scene, vector<DMatch> good_matches, Mat &H, vector<uchar> &inliers) {
    //-- Localize the object
    vector<Point2f> obj;
    vector<Point2f> scene;
    
    for (int i = 0; i < good_matches.size(); i++) {
        //-- Get the keypoints from the good matches
        obj.push_back( keypoints_object[good_matches[i].queryIdx].pt );
        scene.push_back( keypoints_scene[good_matches[i].trainIdx].pt );
    }
    // ORIGINAL OUTPUT
    // printf("Good Matches: %lu\n", good_matches.size());
    
    int inliersCount = 0;
    // cout << "FOUND " << keypoints_object.size() << " keypoints on first image" << endl;
    // cout << "FOUND " << keypoints_scene.size() << " keypoints on second image" << endl;
    
    // Use RANSAC to find homography
    H = findHomography( obj, scene, CV_RANSAC, 3, inliers );
    //cout << "H = "<< endl << " "  << H << endl << endl;
    
    // // Only count non '\0' inliers
    // for (int i = 0; i < inliers.size(); i++) {
    //     if (inliers[i] != '\0') {
    //         inliersCount++;
    //     }
    // }
    
    // printf("-- Inlier size : %d \n\n", inliersCount );
}

void getImageCorners(Mat img_object, vector<Point2f> &object_corners) {
    object_corners[0] = cvPoint( 0, 0 );                                object_corners[1] = cvPoint( img_object.cols, 0 );
    object_corners[2] = cvPoint( img_object.cols, img_object.rows );    object_corners[3] = cvPoint( 0, img_object.rows );
    object_corners[4] = cvPoint( img_object.cols / 2, img_object.rows / 2);
}

bool findRect(vector<Point2f> scene_corners, Rect &rect) {
    int smallestX = 5000;
    int largestX = -1;
    int smallestY = 5000;
    int largestY = -1;
    for (int i = 0; i < 4; i++) {
        if (scene_corners[i].x < smallestX) {
            smallestX = scene_corners[i].x;
        }
        
        if (scene_corners[i].x > largestX) {
            largestX = scene_corners[i].x;
        }
        
        if (scene_corners[i].y < smallestY) {
            smallestY = scene_corners[i].y;
        }
        
        if (scene_corners[i].y > largestY) {
            largestY = scene_corners[i].y;
        }
    }
    
    rect = *new Rect(smallestX, smallestY, largestX - smallestX, largestY - smallestY);
    
    cout << "-- Rect: " << rect.x << " " << rect.y << " " << rect.width << " " << rect.height << endl ;
    
    if (rect.width < 50 || rect.height < 50) {
        return false;
    }
    return true;
}

void drawRects(Mat img_results, vector<Point2f> object_corners, vector<Point2f> scene_corners, Mat img_object) {
    //-- Draw lines between the corners (the object to be mapped - image_1 )
    line( img_results, object_corners[0], object_corners[1], Scalar(0, 255, 0), 2 );
    line( img_results, object_corners[1], object_corners[2], Scalar(0, 255, 0), 2 );
    line( img_results, object_corners[2], object_corners[3], Scalar(0, 255, 0), 2 );
    line( img_results, object_corners[3], object_corners[0], Scalar(0, 255, 0), 2 );
    
    //-- Draw lines between the corners (the mapped object in the scene - image_2 )
    line( img_results, scene_corners[0] + Point2f( img_object.cols, 0), scene_corners[1] + Point2f( img_object.cols, 0), Scalar(0, 255, 0), 2 );
    line( img_results, scene_corners[1] + Point2f( img_object.cols, 0), scene_corners[2] + Point2f( img_object.cols, 0), Scalar( 0, 255, 0), 2 );
    line( img_results, scene_corners[2] + Point2f( img_object.cols, 0), scene_corners[3] + Point2f( img_object.cols, 0), Scalar( 0, 255, 0), 2 );
    line( img_results, scene_corners[3] + Point2f( img_object.cols, 0), scene_corners[0] + Point2f( img_object.cols, 0), Scalar( 0, 255, 0), 2 );
}

void skinColorAdjustment(Mat img_object, Mat &img_skin) {
    Scalar img_object_mean = mean(img_object);
    Scalar img_skin_mean = mean(img_skin);
    img_skin = img_skin - (img_skin_mean - img_object_mean);
    // img_skin.convertTo(img_skin, -1, 0.5, 0);
    // imshow("img_skin_adjusted", img_skin);
}

void skinColorFilter(Mat img_skin, Mat &img_skin_binary) {
    // GaussianBlur(img_skin, img_skin, Size(3,3), 0);
    Mat hsv;
    cvtColor(img_skin, hsv, CV_BGR2HSV);
    // imshow("img_skin", hsv);
    
    // Filter image color
    Scalar skin_lower_threshold = *new Scalar(30, 60, 60); // (70, 60, 50);// (50, 100, 150);
    Scalar skin_upper_threshold = *new Scalar(80, 255, 255); // (100, 90, 70);// (100, 160, 255);
    inRange(hsv, skin_lower_threshold, skin_upper_threshold, img_skin_binary);
    
    // Erode and dilate
    morphologyEx(img_skin_binary, img_skin_binary, CV_MOP_ERODE, Mat1b(3,3,1));
    morphologyEx(img_skin_binary, img_skin_binary, CV_MOP_DILATE, Mat1b(5,5,1));
    
    imshow("img_skin_binary", img_skin_binary);
}

void skinColorSubtractor(Mat img_skin, Mat img_object, Mat &img_skin_binary) {
    absdiff(img_object, img_skin, img_skin_binary);
    // imshow("img_skin_binary_before", img_skin_binary);
    
    cvtColor(img_skin_binary, img_skin_binary, COLOR_RGB2GRAY);
    
    // Potential: add parallel calculation of different threshlds
    threshold(img_skin_binary, img_skin_binary, 80, 255, CV_THRESH_BINARY);
    
    // Erode and dilate
    morphologyEx(img_skin_binary, img_skin_binary, CV_MOP_ERODE, Mat1b(3,3,1));
    morphologyEx(img_skin_binary, img_skin_binary, CV_MOP_DILATE, Mat1b(3,3,1));
    
    // imshow("img_skin_binary_after", img_skin_binary);
}


int angleBetween(Point tip, Point next, Point prev) {
    // calculate the angle between the tip and its neighboring folds
    // (in integer degrees)
    return abs((int) round((atan2(next.x - tip.x, next.y - tip.y) - atan2(prev.x - tip.x, prev.y - tip.y))*180/PI));
}

void findConvexHullBasedFingertipLocation(vector<vector<Point> > contours, vector<vector<Point> > hull, int index_of_biggest_contour, double avg_button_size, vector<Point2f> &fingertip_location, Mat &draw_contours) {
    // Find the convex hull object for each contour
    for (int i = 0; i < contours.size(); i++){
        //find the hulls
        convexHull(contours[i], hull[i], true);
    }
    
    // Draw contours + hull results
    Scalar color = Scalar( 255, 0, 0 );
    drawContours( draw_contours, contours, index_of_biggest_contour, color, 2, 8, vector<Vec4i>(), 0, Point() );
    drawContours( draw_contours, hull, index_of_biggest_contour, color, 2, 8, vector<Vec4i>(), 0, Point() );
    
    // Find and draw fingertip location - Based on Convex Hull
    double smallest_y = 3000;
    int smallest_y_hull_idx = -1;
    for (int i = 0; i < hull[index_of_biggest_contour].size(); i++) {
        Point pt = hull[index_of_biggest_contour][i];
        
        if (pt.y < smallest_y) {
            smallest_y = pt.y;
            smallest_y_hull_idx = i;
        }
    }
    
    double offset = avg_button_size / 2 * 0.5;
    fingertip_location[0].x = hull[index_of_biggest_contour][smallest_y_hull_idx].x;
    fingertip_location[0].y = hull[index_of_biggest_contour][smallest_y_hull_idx].y + offset; // move fingertip location y down into the finger
    
    circle(draw_contours, fingertip_location[0], 7, Scalar(0,255, 0), 2, 8, 0);
    
    /*
     int previous_smallest_y_hull_id, next_smallest_y_hull_id;
     
     if (smallest_y_hull_idx != -1) {
     previous_smallest_y_hull_id = smallest_y_hull_idx - 1 < 0 ? hull[index_of_biggest_contour].size() - 1 : smallest_y_hull_idx - 1;
     next_smallest_y_hull_id = smallest_y_hull_idx + 1 > hull[index_of_biggest_contour].size() - 1 ? 0 : smallest_y_hull_idx + 1;
     
     // circle(draw_contours, hull[index_of_biggest_contour][smallest_y_hull_idx], 7, Scalar(0,255, 0), 2, 8, 0);
     circle(draw_contours, hull[index_of_biggest_contour][previous_smallest_y_hull_id], 7, Scalar(255,0, 0), 2, 8, 0);
     circle(draw_contours, hull[index_of_biggest_contour][next_smallest_y_hull_id], 7, Scalar(0,0, 255), 2, 8, 0);
     }
     
     double dist_previous_current = sqrt((hull[index_of_biggest_contour][previous_smallest_y_hull_id].x - hull[index_of_biggest_contour][smallest_y_hull_idx].x) *
     (hull[index_of_biggest_contour][previous_smallest_y_hull_id].x - hull[index_of_biggest_contour][smallest_y_hull_idx].x) +
     (hull[index_of_biggest_contour][previous_smallest_y_hull_id].y - hull[index_of_biggest_contour][smallest_y_hull_idx].y) *
     (hull[index_of_biggest_contour][previous_smallest_y_hull_id].y - hull[index_of_biggest_contour][smallest_y_hull_idx].y) );
     
     double dist_next_current = sqrt((hull[index_of_biggest_contour][next_smallest_y_hull_id].x - hull[index_of_biggest_contour][smallest_y_hull_idx].x) *
     (hull[index_of_biggest_contour][next_smallest_y_hull_id].x - hull[index_of_biggest_contour][smallest_y_hull_idx].x) +
     (hull[index_of_biggest_contour][next_smallest_y_hull_id].y - hull[index_of_biggest_contour][smallest_y_hull_idx].y) *
     (hull[index_of_biggest_contour][next_smallest_y_hull_id].y - hull[index_of_biggest_contour][smallest_y_hull_idx].y) );
     
     // smallest y being the left most point
     if (dist_previous_current > dist_next_current) {
     fingertip_location[0].x = hull[index_of_biggest_contour][smallest_y_hull_idx].x + offset;
     fingertip_location[0].y = hull[index_of_biggest_contour][smallest_y_hull_idx].y + offset;
     }
     // smallest y being the right most point
     else {
     fingertip_location[0].x = hull[index_of_biggest_contour][smallest_y_hull_idx].x - offset;
     fingertip_location[0].y = hull[index_of_biggest_contour][smallest_y_hull_idx].y + offset;
     }
     */
}


void findConvexHullDefectBasedFingertipLocation(vector<vector<Point> > contours, vector<vector<Point> > hull, int index_of_biggest_contour, double avg_button_size, vector<Point2f> &fingertip_location, Mat &draw_contours) {
    //Find the convex hull and defects object for each contour
    vector<vector<int> > hulls_I(contours.size());
    vector<vector<Vec4i> > defects(contours.size());
    vector<vector<Point> > defect_points(contours.size());
    
    for (int i = 0; i <contours.size(); ++i){
        //find the hulls
        convexHull(contours[i], hulls_I[i], true);
        convexHull(contours[i], hull[i], true);
        //find the defects
        if (contours[i].size() > 3 ) {
            convexityDefects(contours[i], hulls_I[i], defects[i]);
        }
    }
    
    //Draw contours + hull results
    Scalar color = Scalar( 255, 0, 0 );
    drawContours( draw_contours, contours, index_of_biggest_contour, color, 1, 8, vector<Vec4i>(), 0, Point() );
    drawContours( draw_contours, hull, index_of_biggest_contour, color, 1, 8, vector<Vec4i>(), 0, Point() );
    
    
    vector<Point> tip_pts;
    vector<Point> fold_pts;
    vector<int> depths;
    vector<Point> fingertips;
    
    for (int k = 0; k < defects[index_of_biggest_contour].size(); k++) {
        int ind_0 = defects[index_of_biggest_contour][k][0];
        int ind_1 = defects[index_of_biggest_contour][k][1];
        int ind_2 = defects[index_of_biggest_contour][k][2];
        tip_pts.push_back(contours[index_of_biggest_contour][ind_0]);
        tip_pts.push_back(contours[index_of_biggest_contour][ind_1]);
        fold_pts.push_back(contours[index_of_biggest_contour][ind_2]);
        depths.push_back(defects[index_of_biggest_contour][k][3]);
    }
    
    int numPoints = (int)fold_pts.size();
    for (int i=0; i < numPoints; i++) {
        if (depths[i] < MIN_FINGER_DEPTH)
            continue;
        
        // look at fold points on either side of a tip
        int pdx = (i == 0) ? (numPoints-1) : (i - 1);
        int sdx = (i == numPoints-1) ? 0 : (i + 1);
        
        int angle = angleBetween(tip_pts[i], fold_pts[pdx], fold_pts[sdx]);
        cout << angle << " ";
        if (angle >= MAX_FINGER_ANGLE)   // angle between finger and folds too wide
            continue;
        
        // this point is probably a fingertip, so add to list
        fingertips.push_back(tip_pts[i]);
    }
    
    for (int i = 0; i < fingertips.size(); i ++) {
        //circle(drawing,fingerTips[i],5,Scalar(0,0,255),-1);
    }
    
    //Find and draw fingertip location - Based on Convex Hull and Defects
    double smallest_y_fingertip = 3000;
    int smallest_y_fingertip_idx = -1;
    for (int i = 0; i < fingertips.size(); i++) {
        Point pt = fingertips[i];
        if (pt.y < smallest_y_fingertip) {
            smallest_y_fingertip = pt.y;
            smallest_y_fingertip_idx = i;
        }
    }
    if (smallest_y_fingertip_idx != -1) {
        circle(draw_contours, fingertips[smallest_y_fingertip_idx], 10, Scalar(0,255,0));
    }
    
    fingertip_location[0].x = hull[index_of_biggest_contour][smallest_y_fingertip_idx].x;
    fingertip_location[0].y = hull[index_of_biggest_contour][smallest_y_fingertip_idx].y;
}


void findFingertipLocation (Mat img_skin_binary, int area, Mat H, double avg_button_size, vector<Point2f> &fingertip_location) {
    Mat canny_output;
    vector<vector<Point> > contours;
    vector<Vec4i> hierarchy;
    findContours( img_skin_binary, contours, hierarchy, CV_RETR_TREE, CV_CHAIN_APPROX_SIMPLE, Point(0, 0) );
    
    int index_of_biggest_contour = -1;
    double size_of_biggest_contour = 0;
    for (int i = 0; i < contours.size(); i++) {
        if (contourArea(contours[i]) > size_of_biggest_contour) {
            size_of_biggest_contour = contourArea(contours[i]);
            index_of_biggest_contour = i;
        }
    }
    
    cout << "-- sizeOfBiggestContour = " << size_of_biggest_contour << endl;
    cout << "-- indexOfBiggestContour = " << index_of_biggest_contour << endl << endl;
    
    try {
        if (size_of_biggest_contour > 0.0002 * area && index_of_biggest_contour >= 0 && index_of_biggest_contour < contours.size()) {
            Moments mu = moments(contours[index_of_biggest_contour], false);
            fingertip_location[0].x = mu.m10 / mu.m00;
            fingertip_location[0].y = mu.m01 / mu.m00;
        }
    } catch (Exception &e) {
    }

    /*
    //0.002
    if (size_of_biggest_contour > 0.002 * area) {
        Mat draw_contours = Mat::zeros( img_skin_binary.size(), CV_8UC3 );
        
        vector<vector<Point> > hull(contours.size());
        findConvexHullBasedFingertipLocation(contours, hull, index_of_biggest_contour, avg_button_size, fingertip_location, draw_contours);
        // findConvexHullDefectBasedFingertipLocation(contours, hull, index_of_biggest_contour, fingertip_location, draw_contours);
        
        // imshow("Contours", draw_contours);
    }
    */
}

bool checkNewImgFromPool(vector<Mat> &new_img_pool, vector<GpuMat> &new_img_pool_gpu, vector<vector<KeyPoint>> &new_keypoints_pool, vector<GpuMat> &new_keypoints_pool_gpu, vector<Mat> &new_descriptors_pool, vector<GpuMat> &new_descriptors_pool_gpu, 
    GpuMat &img_scene_gpu, vector<KeyPoint> &keypoints_scene, GpuMat &descriptors_scene_gpu) {

    if (new_img_pool_gpu.size() <= 1) 
        return false;

    int matched = 0;
    int max_matched_id = 0;

    img_scene_gpu = new_img_pool_gpu.back();
    keypoints_scene = new_keypoints_pool.back();
    descriptors_scene_gpu = new_descriptors_pool_gpu.back();

    BFMatcher_GPU matcher(NORM_L2);
    GpuMat trainIdx, distance;
               
    for (int i = 0; i < new_img_pool_gpu.size() - 1; i++) { 
        // // CPU ONLY
        // vector<DMatch> matches;
        // // matcher.match(reachable_descriptors_object_gpu_vector[i], descriptors_scene, matches);
        // matcher.match(new_descriptors_pool[i], descriptors_scene, matches);
        
        matcher.matchSingle(new_descriptors_pool_gpu[i], descriptors_scene_gpu, trainIdx, distance);
        vector<DMatch> matches;
        BFMatcher_GPU::matchDownload(trainIdx, distance, matches);

        vector< DMatch > tmp_good_matches;
        gpuFindGoodMatches(matches, tmp_good_matches);

        // DEBUG
        if (tmp_good_matches.size() < 4) {
            cout << "-- Skipping Image -- " << i << endl;
            continue;
        }
                
        // Find H
        Mat tempH;
        vector<uchar> inliers;
        findH(new_keypoints_pool[i], keypoints_scene, tmp_good_matches, tempH, inliers);
                
        // Only count non '\0' inliers
        int inliersCount = 0;
        for (int j = 0; j < inliers.size(); j++) {
            if (inliers[j] != '\0') {
                inliersCount++;
            }
        }

        float currRatio = inliersCount / (float)tmp_good_matches.size(); //  / ((float)keypoints_object_vector[i].size() + (float)keypoints_scene.size());
        // cout << "Ratio of Inlier: " << currRatio << endl;
        // cout << endl;
        if (currRatio >= 0.7) {
            matched++;
            max_matched_id = i;
        }
    }

    int threshold = 2;
    if (matched >= threshold) {
        new_img_pool.clear();
        new_img_pool_gpu.clear();
        new_keypoints_pool.clear();
        new_keypoints_pool_gpu.clear();
        new_descriptors_pool.clear();
        new_descriptors_pool_gpu.clear();
        return true;
    }

    return false;

}


#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <sstream>
#include <fstream>
#include <math.h>
#include <vector>
#include <string>
#include <time.h>
#include <algorithm>
#include <string> 

#include "opencv2/core/core.hpp"
#include "opencv2/features2d/features2d.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/calib3d/calib3d.hpp"
#include "opencv2/nonfree/nonfree.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/video/background_segm.hpp"
#include <opencv2/gpu/gpu.hpp>
#include "opencv2/nonfree/gpu.hpp"
#include "sqlite3.h"

// TODO: define envpaths
// #define PATH genenv("APP_PATH")
#define DB "/home/guoanhong/Desktop/ApplianceReader/WebServer/db/ApplianceReader.db"
#define refImageDirBase "/home/guoanhong/Desktop/ApplianceReader/WebServer/images_reference/microwave-crop.jpg"
#define imageDirBase "/home/guoanhong/Desktop/ApplianceReader/WebServer/images_video/"
#define session "microwave"

using namespace cv;
using namespace std;
using namespace cv::gpu;

const int MIN_FINGER_DEPTH = 20;
const int MAX_FINGER_ANGLE = 60;
const double PI = 3.1415926;

bool isOpenDB = false;
sqlite3 *dbfile;

string id = "0";
string image = "";
string mode = "0";
string target = "";
string feedback = "";
string guidance = "";
int idle = 1;

bool ConnectDB ();
void DisonnectDB ();

void getImageToProcess();
int updateImage(int objectfound, int fingerfound, int fingerx, int fingery);
int skipImage();

void readme();
void labels_hash(Mat img_object, double ratio, vector<vector<int> > &button_map, vector<string> &true_labels, vector<vector<double> > &button_center, double &avg_button_size);
int skipFrames(VideoCapture capture, int num);
void lightingDetection(Mat img);
void gpuFindGoodMatches(vector< DMatch > matches, vector< DMatch > &good_matches);
void findH(vector<KeyPoint> keypoints_object, vector<KeyPoint> keypoints_scene, vector<DMatch> good_matches, Mat &H, vector<uchar> &inliers);
void getImageCorners(Mat img_object, vector<Point2f> &object_corners);
bool findRect(vector<Point2f> scene_corners, Rect &rect);
void drawRects(Mat img_results, vector<Point2f> object_corners, vector<Point2f> scene_corners, Mat img_object);
void skinColorAdjustment(Mat img_object, Mat &img_skin);
void skinColorFilter(Mat img_skin, Mat &img_skin_binary);
void skinColorSubtractor(Mat img_skin, Mat img_object, Mat &img_skin_binary);
void findFingertipLocation (Mat img_skin_binary, int area, Mat H, double avg_button_size, vector<Point2f> &fingertip_location);
string getFeedback(vector<vector<int> > button_map, vector<string> true_labels, vector<Point2f> fingertip_location, vector<Point2f> previous_fingertip_location, double avg_button_size);
int getGuidanceIndex(vector<string> true_labels, string targets);
string getGuidance(vector<vector<int> > button_map, vector<string> true_labels, vector<double> guidance_button_center, vector<Point2f> fingertip_location, vector<Point2f> previous_fingertip_location, double avg_button_size);
string getDirection(vector<double> guidance_button_center, vector<Point2f> fingertip_location, double avg_button_size);
void anounceFeedback(string feedback);
void findConvexHullBasedFingertipLocation(vector<vector<Point> > contours, vector<vector<Point> > hull, int index_of_biggest_contour, double avg_button_size, vector<Point2f> &fingertip_location, Mat &draw_contours);
void findConvexHullDefectBasedFingertipLocation(vector<vector<Point> > contours, vector<vector<Point> > hull, int index_of_biggest_contour, double avg_button_size, vector<Point2f> &fingertip_location, Mat &draw_contours);
int angleBetween(Point tip, Point next, Point prev);
void drawFingertips(Mat img_results, Mat img_object, vector<Point2f> fingertip_location, vector<Point2f> scene_fingertip_location);

/** @function main */
int main(int argc, char** argv)
{
    cout << "Start (GPU sqlite)"<< endl;
    
    isOpenDB = ConnectDB();
    
    if (isOpenDB) cout << "Connected Successful" << endl;
    else cout << "connection failed " << endl;
    
    //    cout << "Please input button index for guidance: ";
    //    int guidance_index = -1;
    //    scanf("%d", &guidance_index);
    
    Mat img_object = imread(refImageDirBase);
    
    // Size img_object_size(150, 445); //(150, 445)
    // resize(img_object, img_object, img_object_size);
    
    double ratio = img_object.rows > img_object.cols ? 600.0 / (double)img_object.rows : 600.0 / (double) img_object.cols;
    Size img_object_size(img_object.cols * ratio, img_object.rows * ratio); //(150, 445)
    resize(img_object, img_object, img_object_size);
    
    vector<vector<int> > button_map;
    vector<string> true_labels;
    vector<vector<double> > button_center;
    double avg_button_size;
    labels_hash(img_object, ratio, button_map, true_labels, button_center, avg_button_size);
    
    lightingDetection(img_object);
    
    // Computer SURF keypoints and descriptors for object (reference) image
    GpuMat img_object_gpu;
    Mat img_object_gray;
    cvtColor(img_object, img_object_gray, CV_RGB2GRAY);
    img_object_gpu.upload(img_object_gray);
    cv::gpu::printShortCudaDeviceInfo(cv::gpu::getDevice());
    
    SURF_GPU surf;
    surf.hessianThreshold = 400;
    
    GpuMat keypoints_object_gpu, keypoints_scene_gpu;
    GpuMat descriptors_object_gpu, descriptors_scene_gpu;
    surf(img_object_gpu, GpuMat(), keypoints_object_gpu, descriptors_object_gpu);
    cout << "FOUND " << keypoints_object_gpu.cols << " keypoints on first image" << endl;
    
    int count = 0;
    clock_t last_time = clock();
    printf("Gran = %f\n", 0.05 * 1000000);
    
    string previous_guidance = "";
    
    vector<Point2f> previous_fingertip_location(1);
    previous_fingertip_location[0].x = -1;
    previous_fingertip_location[0].y = -1;
    
    int object_not_found = 1;
    int fingertip_not_found = 1;
    int fingertip_found = 1;
    int provide_guidance = 1;
    int no_button = 1;

    int db_object_found = -1;
    int db_finger_found = -1;
    int fingerx = -1;
    int fingery = -1;
    
    for ( ; ; ) {
        try {
            while (idle == 1) {
                if(clock() - last_time > (double)(0.05 * 1000000)) {
                    count++;
                    getImageToProcess();
                    last_time = clock();
                }
            }
            
            Mat img_scene = imread(imageDirBase+image);
            Size img_scene_size(360, 640);
            
            if (img_scene.empty()) break;
            
            // resize(img_scene, img_scene, img_scene_size);
            
            cout << "--Size of frame is: "<< img_scene.cols << " " << img_scene.rows << endl;
            
            lightingDetection(img_scene);
            
            /****** START CLOCK *******/
            clock_t tic = clock();
            
            // Computer SURF keypoints and descriptors for scene (video) image
            GpuMat img_scene_gpu;
            Mat img_scene_gray;
            cvtColor(img_scene, img_scene_gray, CV_RGB2GRAY);
            img_scene_gpu.upload(img_scene_gray);
            surf(img_scene_gpu, GpuMat(), keypoints_scene_gpu, descriptors_scene_gpu);
            cout << "FOUND " << keypoints_scene_gpu.cols << " keypoints on second image" << endl;
            
            if (keypoints_scene_gpu.cols == 0) {
                idle = 1;
                skipImage();
                continue;
            }

            // matching descriptors
            BFMatcher_GPU matcher(NORM_L2);
            GpuMat trainIdx, distance;
            matcher.matchSingle(descriptors_object_gpu, descriptors_scene_gpu, trainIdx, distance);
            
            // downloading results
            vector<KeyPoint> keypoints_object, keypoints_scene;
            vector<float> descriptors_object, descriptors_scene;
            vector<DMatch> matches;
            surf.downloadKeypoints(keypoints_object_gpu, keypoints_object);
            surf.downloadDescriptors(descriptors_object_gpu, descriptors_object);
            surf.downloadKeypoints(keypoints_scene_gpu, keypoints_scene);
            surf.downloadDescriptors(descriptors_scene_gpu, descriptors_scene);
            BFMatcher_GPU::matchDownload(trainIdx, distance, matches);
            
            vector< DMatch > good_matches;
            gpuFindGoodMatches(matches, good_matches);
            
            Mat img_results;
            /* drawMatches( img_object, keypoints_object, img_scene, keypoints_scene,
             good_matches, img_results, Scalar::all(-1), Scalar::all(-1),
             vector<char>(), DrawMatchesFlags::NOT_DRAW_SINGLE_POINTS ); */
            
            // Find H
            Mat H;
            vector<uchar> inliers;
            findH(keypoints_object, keypoints_scene, good_matches, H, inliers);
            
            // Get the corners from the object (reference) image
            vector<Point2f> object_corners(5);
            getImageCorners(img_object, object_corners);
            
            // Transform object corners to scene corners using H
            vector<Point2f> scene_corners(5);
            perspectiveTransform( object_corners, scene_corners, H);
            
            int guidance_index = -2;
            if (stoi(mode) == 2) {
                guidance_index = getGuidanceIndex(true_labels, target);
            }
            
            if (guidance_index == -1) {
                if (no_button % 5 == 1) {
                    feedback = "no button";
                } else {
                    feedback = "";
                }
                no_button++;
            } else {
                no_button = 1;

                Rect rect;
                if (!findRect(scene_corners, rect)) {
                    db_object_found = 0;
                    db_finger_found = -1;
                    fingerx = -1;
                    fingery = -1;

                    fingertip_not_found = 0;
                    fingertip_found = 1;
                    provide_guidance = 1;
                    
                    previous_fingertip_location[0].x = -1;
                    previous_fingertip_location[0].y = -1;
                    
                    if (object_not_found % 5 == 0) {
                        feedback = "no object";
                        object_not_found++;
                    } else {
                        feedback = "";
                        object_not_found++;
                    }
                    
                    cout << "No Object" << endl ;
                } else {
                    db_object_found = 1;

                    object_not_found = 1;
                    
                    // If found, draw rectangles around both images
                    drawRects(img_results, object_corners, scene_corners, img_object);
                    
                    // Warp scene image with finger to reference image size
                    Mat img_skin;
                    warpPerspective(img_scene, img_skin, H.inv(), img_object.size(), INTER_LINEAR, BORDER_CONSTANT);
                    
                    // Filter skin color to get binary image
                    Mat img_skin_binary;
                    skinColorAdjustment(img_object, img_skin);
                    skinColorFilter(img_skin, img_skin_binary);
                    // skinColorSubtractor(img_skin, img_object, img_skin_binary);
                    
                    // Find fingertip location in reference image
                    vector<Point2f> fingertip_location(1);
                    fingertip_location[0].x = -1;
                    fingertip_location[0].y = -1;
                    findFingertipLocation(img_skin_binary, img_object.rows * img_object.cols, H, avg_button_size, fingertip_location);
                    
                    if (fingertip_location[0].x == -1 && fingertip_location[0].y == -1) {
                        Point2f object_center_scene;
                        object_center_scene = scene_corners[4];

                        feedback = "";

                        if (object_center_scene.y < 0) {
                            feedback += "up ";
                        } else if (object_center_scene.y > img_scene.rows) {
                            feedback += "down ";
                        }
                        if (object_center_scene.x < 0) {
                            feedback += "right";
                        } else if (object_center_scene.x > img_scene.cols) {
                            feedback += "left";
                        }

                        if (feedback.compare("")) {
                            feedback = "Move phone to " + feedback;
                        } else {
                            db_finger_found = 0;
                            fingerx = -1;
                            fingery = -1;

                            previous_fingertip_location[0].x = -1;
                            previous_fingertip_location[0].y = -1;
                            fingertip_found = 1;
                            provide_guidance = 1;
                            
                            if (fingertip_not_found % 5 == 0) {
                                feedback = "no finger";
                                fingertip_not_found++;
                            } else {
                                feedback = "";
                                fingertip_not_found++;
                            }          
                        }
                    } else {
                        db_finger_found = 1;
                        fingerx = (int) fingertip_location[0].x;
                        fingery = (int) fingertip_location[0].y;
                        
                        switch (stoi(mode)) {
                            case 1:
                                // Provide feedback
                                if (fingertip_found % 5 == 0) {
                                    previous_fingertip_location[0].x = -1;
                                    previous_fingertip_location[0].y = -1;
                                    fingertip_found++;
                                } else {
                                    fingertip_found++;
                                }
                                fingertip_not_found = 1;
                                feedback = getFeedback(button_map, true_labels, fingertip_location, previous_fingertip_location, avg_button_size);
                                break;
                                
                            case 2: {
                                // Provide guidance
                                if (guidance_index != -1) {
                                    previous_guidance = guidance;
                                    guidance = getGuidance(button_map, true_labels, button_center[guidance_index], fingertip_location, previous_fingertip_location, avg_button_size);
                                }
                                
                                if (guidance.compare(previous_guidance)) {
                                    provide_guidance = 1;
                                    feedback = guidance;
                                    // feedback = feedback + getFeedback(button_map, true_labels, fingertip_location, previous_fingertip_location);
                                } else {
                                    if (provide_guidance % 5 == 1) {
                                        feedback = guidance;
                                        provide_guidance++;
                                    } else {
                                        feedback = "";
                                        provide_guidance++;
                                    }
                                }
                                break;
                            }
                        }
                        
                        previous_fingertip_location = fingertip_location;
                    }
                    
                    // Find relative fingertip location in scene image
                    vector<Point2f> scene_fingertip_location(1);
                    perspectiveTransform( fingertip_location, scene_fingertip_location, H);
                    
                    printf("-- Object Test Point : %f %f \n", fingertip_location[0].x, fingertip_location[0].y);
                    printf("-- Scene Test Point : %f %f \n", scene_fingertip_location[0].x, scene_fingertip_location[0].y);
                    
                    //smooth fingertip location - no need, just filter out unreasonable points
                    //remove rightmost fingertip location - cut side part of reference image solves this problem
                    
                    drawFingertips(img_results, img_object, fingertip_location, scene_fingertip_location);
                }
            }
        
            //-- Show detected matches
            // imshow( "Overall Results", img_results );

            // anounceFeedback(feedback);
            
            cout << "Feedback: " << feedback << endl;
            
            clock_t toc = clock();
            printf("Elapsed: %f seconds\n", (double)(toc - tic) / CLOCKS_PER_SEC);
            /****** END CLOCK *******/
            
            updateImage(db_object_found, db_finger_found, fingerx, fingery);
            idle = 1;
            
            waitKey(1); // waits to display frame
        } catch (Exception &e) {
            idle = 1;
            skipImage();
            continue;
        }
    }
    
    waitKey(0);
    return 0;
}

bool ConnectDB () {
    if ( sqlite3_open(DB, &dbfile) == SQLITE_OK ) {
        isOpenDB = true;
        return true;
    }
    return false;
}

void DisonnectDB () {
    if ( isOpenDB == true ) {
        sqlite3_close(dbfile);
    }
}

void getImageToProcess() {
    sqlite3_stmt *statement;
    
    char query[1000];
    
    sprintf(query, "SELECT image, id, imageactive, target FROM images_video WHERE imageactive>0 AND id>%s ORDER BY id DESC", id.c_str());

    // cout << query << endl;
    
    if ( sqlite3_prepare(dbfile, query, -1, &statement, 0 ) == SQLITE_OK ) {
        int firstResult = sqlite3_step(statement);
        if (firstResult == 100) {
            cout << "Got it." << endl;
            image = (char*)sqlite3_column_text(statement, 0);
            cout << image << endl;
            id = (char*)sqlite3_column_text(statement, 1);
            cout << id << endl;
            mode = (char*)sqlite3_column_text(statement, 2);
            cout << mode << endl;
            target = (char*)sqlite3_column_text(statement, 3);
            cout << target << endl;
            idle = 0;
        } else if (firstResult == 101) {
            // cout << "Nothing." << endl;
        }
        sqlite3_finalize(statement);
    }
}

int updateImage(int objectfound, int fingerfound, int fingerx, int fingery) {
    sqlite3_stmt *statement;
    int result;
    
    char query[1000];
    
    sprintf(query, "UPDATE images_video SET imageactive=0,"
            "feedback=\"%s\",feedbackactive=1,resulttime=current_timestamp, objectfound=%s, fingerfound=%s, fingertipx=%s, fingertipy=%s WHERE id=%s",
            feedback.c_str(), to_string(objectfound).c_str(), to_string(fingerfound).c_str(), to_string(fingerx).c_str(), to_string(fingery).c_str(), id.c_str());
    
    cout << query << endl;
    
    {
        if(sqlite3_prepare(dbfile,query,-1,&statement,0)==SQLITE_OK) {
            int res = sqlite3_step(statement);
            result = res;
            sqlite3_finalize(statement);
            
            cout << "Update Image!" << endl;
        }
        return result;
    }
    
    cout << "Update Image!" << endl;
    
    return 0;
}

int skipImage() {
    sqlite3_stmt *statement;
    int result;
    
    char query[1000];
    
    sprintf(query, "UPDATE images_video SET imageactive=0 WHERE id=%s", id.c_str());
    
    cout << query << endl;
    
    {
        if(sqlite3_prepare(dbfile,query,-1,&statement,0)==SQLITE_OK) {
            int res = sqlite3_step(statement);
            result = res;
            sqlite3_finalize(statement);
            
            cout << "Skip Image!" << endl;
        }
        return result;
    }
    
    cout << "Skip Image!" << endl;
    
    return 0;
}

void readme(){
    cout << " Usage: ./ApplianceReader <reference image> <scene video> <labels.txt>" << endl;
}

std::vector<std::string> &split(const std::string &s, char delim, std::vector<std::string> &elems) {
    std::stringstream ss(s);
    std::string item;
    while (std::getline(ss, item, delim)) {
        elems.push_back(item);
    }
    return elems;
}

std::vector<std::string> split(const std::string &s, char delim) {
    std::vector<std::string> elems;
    split(s, delim, elems);
    return elems;
}

void labels_hash(Mat img_object, double ratio, vector<vector<int> > &button_map, vector<string> &true_labels, vector<vector<double> > &button_center, double &avg_button_size) {
    string x1, y1, x2, y2, label;
    vector<vector<string> > labels;
    sqlite3_stmt *statement;
    
    char query[1000];
    sprintf(query, "SELECT x1, y1, x2, y2, label FROM object_buttons WHERE session=\"%s\"", session);
    cout << query << endl;
    
    if ( sqlite3_prepare(dbfile, query, -1, &statement, 0 ) == SQLITE_OK ) {
        
        for (;;) {
            int nextResult = sqlite3_step(statement);
            
            if (nextResult == SQLITE_DONE)
                break;
            
            if (nextResult != SQLITE_ROW) {
                printf("error: %s!\n", sqlite3_errmsg(dbfile));
                break;
            }
            
            cout << "Got it." << endl;
            x1 = (char*)sqlite3_column_text(statement, 0);
            cout << x1 << endl;
            y1 = (char*)sqlite3_column_text(statement, 1);
            cout << y1 << endl;
            x2 = (char*)sqlite3_column_text(statement, 2);
            cout << x1 << endl;
            y2 = (char*)sqlite3_column_text(statement, 3);
            cout << y1 << endl;
            label = (char*)sqlite3_column_text(statement, 4);
            cout << label << endl;
            
            // push x1, y1, x2, y2, label into x and push to labels
            vector<string> x;
            x.push_back(x1);
            x.push_back(y1);
            x.push_back(x2);
            x.push_back(y2);
            x.push_back(label);
            labels.push_back(x);
        }
        
        sqlite3_finalize(statement);
    } else {
        cout << "Failed" << endl;
    }
    
    double sum_button_width = 0;
    
    // compute the centers of each button
    for (int i = 0; i < labels.size(); i++) {
        vector<string> tmp = labels[i];
        double x1 = stod(tmp[0]) * ratio;
        double y1 = stod(tmp[1]) * ratio;
        double x2 = stod(tmp[2]) * ratio;
        double y2 = stod(tmp[3]) * ratio;
        double centerX = (x1 + x2) / 2;
        double centerY = (y1 + y2) / 2;
        vector<double> append = {centerX, centerY};
        button_center.push_back(append);
        true_labels.push_back(tmp[4]);  // push button labels
        
        sum_button_width += x2 - x1;
    }
    
    avg_button_size = sum_button_width / labels.size();
    
    double threshold = avg_button_size; //55
    cout << avg_button_size << endl;
    // Generate two dimensional array, assign class label
    cout << img_object.rows << " " << img_object.cols << endl;
    for (int i = 0; i < img_object.rows; i++) {
        vector<int> row;
        for (int j = 0; j < img_object.cols; j++) {
            int distance[labels.size()];
            int idx_min1 = -1;     double min_d1 = 100000;
            int idx_min2 = -1;     double min_d2 = 100000;
            for (int k = 0; k < labels.size(); k++) {
                double centerX = button_center[k][0];
                double centerY = button_center[k][1];
                distance[k] = sqrt((centerX - j) * (centerX - j) + (centerY - i) * (centerY - i));
                if (distance[k] < min_d2) {
                    if (distance[k] < min_d1) {
                        min_d2 = min_d1;
                        min_d1 = distance[k];
                        idx_min2 = idx_min1;
                        idx_min1 = k;
                    } else {
                        min_d2 = distance[k];
                        idx_min2 = k;
                    }
                }
            }
            
            // check whether in the smallest distance
            vector<string> tmp = labels[idx_min1];
            double x1 = stod(tmp[0]) * ratio;
            double y1 = stod(tmp[1]) * ratio;
            double x2 = stod(tmp[2]) * ratio;
            double y2 = stod(tmp[3]) * ratio;
            
            if (x1 <= j && j <= x2 && i >= y1 && i <= y2) {
                row.push_back(idx_min1);
            } else if (min_d1 > threshold) {
                row.push_back(-1);
            } else if (min_d2 > threshold) {
                row.push_back(idx_min1 + 10000);
            } else {
                row.push_back(idx_min1 * 100 + idx_min2);
            }
        }
        button_map.push_back(row);
    }
    
    // Print out button map for verification
    //    for (int i = 0; i < img_object.rows; i++) {
    //        for (int j = 0; j < img_object.cols; j++) {
    //            cout << button_map[i][j] << " ";
    //        }
    //        cout << "end" << endl;
    //    }
}


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
    
    printf("-- Max dist : %f \n", max_dist );
    printf("-- Min dist : %f \n\n", min_dist );
    
    //-- Draw only "good" matches (i.e. whose distance is less than 3*min_dist )
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
    printf("Good Matches: %lu\n", good_matches.size());
    
    int inliersCount = 0;
    cout << "FOUND " << keypoints_object.size() << " keypoints on first image" << endl;
    cout << "FOUND " << keypoints_scene.size() << " keypoints on second image" << endl;
    
    // Use RANSAC to find homography
    H = findHomography( obj, scene, CV_RANSAC, 3, inliers );
    //cout << "H = "<< endl << " "  << H << endl << endl;
    
    // Only count non '\0' inliers
    for (int i = 0; i < inliers.size(); i++) {
        if (inliers[i] != '\0') {
            inliersCount++;
        }
    }
    
    printf("-- Inlier size : %d \n\n", inliersCount );
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
    GaussianBlur(img_skin, img_skin, Size(3,3), 0);
    Mat hsv;
    cvtColor(img_skin, hsv, CV_BGR2HSV);
    // imshow("img_skin", hsv);
    
    // Filter image color
    Scalar skin_lower_threshold = *new Scalar(0, 90, 60);
    Scalar skin_upper_threshold = *new Scalar(20, 150, 255);
    inRange(hsv, skin_lower_threshold, skin_upper_threshold, img_skin_binary);
    
    // Erode and dilate
    morphologyEx(img_skin_binary, img_skin_binary, CV_MOP_ERODE, Mat1b(3,3,1));
    morphologyEx(img_skin_binary, img_skin_binary, CV_MOP_DILATE, Mat1b(5,5,1));
    
    // imshow("img_skin_binary", img_skin_binary);
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
    
    //0.002
    if (size_of_biggest_contour > 0.002 * area) {
        Mat draw_contours = Mat::zeros( img_skin_binary.size(), CV_8UC3 );
        
        vector<vector<Point> > hull(contours.size());
        findConvexHullBasedFingertipLocation(contours, hull, index_of_biggest_contour, avg_button_size, fingertip_location, draw_contours);
        // findConvexHullDefectBasedFingertipLocation(contours, hull, index_of_biggest_contour, fingertip_location, draw_contours);
        
        // imshow("Contours", draw_contours);
    }
}

string getFeedback(vector<vector<int> > button_map, vector<string> true_labels, vector<Point2f> fingertip_location, vector<Point2f> previous_fingertip_location, double avg_button_size) {
    int y = floor(fingertip_location[0].x);
    int x = floor(fingertip_location[0].y);
    string feedback = "";
    
    int magic_value = button_map[x][y];
    int size = true_labels.size();
    
    if (previous_fingertip_location[0].x == -1 && previous_fingertip_location[0].y == -1) {
        if (magic_value >= 10000) {
            feedback = "near " + true_labels[magic_value - 10000];
        } else if (magic_value > size) {
            feedback = true_labels[magic_value / 100] + " and " + true_labels[magic_value % 100];
        } else if (magic_value >= 0) {
            feedback = true_labels[magic_value];
        } else if (magic_value == -1) {
            feedback = "";
        }
    } else {
        int previous_y = floor(previous_fingertip_location[0].x);
        int previous_x = floor(previous_fingertip_location[0].y);
        int previous_magic_value = button_map[previous_x][previous_y];
        
        double distance = sqrt( (previous_fingertip_location[0].x - fingertip_location[0].x) *
                               (previous_fingertip_location[0].x - fingertip_location[0].x) +
                               (previous_fingertip_location[0].y - fingertip_location[0].y) *
                               (previous_fingertip_location[0].y - fingertip_location[0].y) );
        
        double thres = avg_button_size * 0.3; //10
        
        // Detect movement speed, if fast: say nothing; if slow: use words
        if (distance <= thres) {
            if (magic_value == previous_magic_value) {
                feedback = "";
            } else {
                if (magic_value >= 10000) {
                    feedback = "near " + true_labels[magic_value - 10000];
                } else if (magic_value > size) {
                    feedback = true_labels[magic_value / 100] + " and " + true_labels[magic_value % 100];
                } else if (magic_value >= 0) {
                    feedback = true_labels[magic_value];
                } else if (magic_value == -1) {
                    feedback = "";
                }
            }
        } else {
            if (magic_value >= 10000) {
                feedback = "";
            } else if (magic_value > size) {
                feedback = "";
            } else if (magic_value >= 0) {
                feedback = "";
            } else if (magic_value == -1) {
                feedback = "";
            }
        }
    }
    
    return feedback;
}

int getGuidanceIndex(vector<string> true_labels, string targets) {
    transform(targets.begin(), targets.end(), targets.begin(), ::tolower);

    vector<string> target = split(targets, ';');
    
    for (int i = 0; i < target.size(); i++) {
        for (int j = 0; j < true_labels.size(); j++) {
            if (!true_labels[j].compare(target[i])) {
                return j;
            }
        }
    }
    return -1;
}

string getGuidance(vector<vector<int> > button_map, vector<string> true_labels, vector<double> guidance_button_center, vector<Point2f> fingertip_location, vector<Point2f> previous_fingertip_location, double avg_button_size) {
    string feedback = "";
    
    double guidance_distance = sqrt( (guidance_button_center[0] - fingertip_location[0].x) *
                                    (guidance_button_center[0] - fingertip_location[0].x) +
                                    (guidance_button_center[1] - fingertip_location[0].y) *
                                    (guidance_button_center[1] - fingertip_location[0].y) );
    
    double inner_threshold = avg_button_size * 0.5; //20
    double outer_threshold = avg_button_size * 1.5; //60
    
    if (guidance_distance <= inner_threshold) {
        feedback = getFeedback(button_map, true_labels, fingertip_location, previous_fingertip_location, avg_button_size);
    } else if (guidance_distance <= outer_threshold) {
        feedback = getDirection(guidance_button_center, fingertip_location, avg_button_size);
        feedback = feedback + " slowly";
    } else {
        feedback = getDirection(guidance_button_center, fingertip_location, avg_button_size);
    }
    
    return feedback;
}

string getDirection(vector<double> guidance_button_center, vector<Point2f> fingertip_location, double avg_button_size) {
    string feedback = "";
    
    double threshold = avg_button_size * 0.3; //15
    
    double x_diff = guidance_button_center[0] - fingertip_location[0].x;
    double y_diff = guidance_button_center[1] - fingertip_location[0].y;
    
    // abs(x_diff) for always longest distance
    if (abs(y_diff) > threshold) {
        feedback = y_diff < 0 ? "up" : "down";
    } else {
        feedback = x_diff < 0 ? "left" : "right";
    }
    
    return feedback;
}


void anounceFeedback(string feedback) {
    // Anounce feedback string
    stringstream ss;
    ss << "say \"" << feedback << "\"";
    system(ss.str().c_str());
}

void drawFingertips(Mat img_results, Mat img_object, vector<Point2f> fingertip_location, vector<Point2f> scene_fingertip_location) {
    if (fingertip_location[0].x > 0 && fingertip_location[0].x < img_object.cols &&
        fingertip_location[0].y > 0 && fingertip_location[0].y < img_object.rows) {
        line( img_results, fingertip_location[0] + Point2f(0, 0), scene_fingertip_location[0] + Point2f( img_object.cols, 0), Scalar(0, 255, 0), 4 );
        circle(img_results, fingertip_location[0], 5, Scalar(0,0,255), 4, 8, 0);
        circle(img_results, scene_fingertip_location[0] + Point2f( img_object.cols, 0), 5, Scalar(0,0,255), 4, 8, 0);
    }
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

int angleBetween(Point tip, Point next, Point prev) {
    // calculate the angle between the tip and its neighboring folds
    // (in integer degrees)
    return abs((int) round((atan2(next.x - tip.x, next.y - tip.y) - atan2(prev.x - tip.x, prev.y - tip.y))*180/PI));
}

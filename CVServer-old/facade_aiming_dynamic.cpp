#include <sys/types.h>
#include <dirent.h>
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

#define DB "/home/guoanhong/Desktop/ApplianceReader/WebServer/db/ApplianceReader.db"
#define refImageDirBase "/home/guoanhong/Desktop/ApplianceReader/WebServer/images_reference/"
#define imageDirBase "/home/guoanhong/Desktop/ApplianceReader/WebServer/images_video/"
#define session "dollarbill"

using namespace cv;
using namespace std;
using namespace cv::gpu;

bool isOpenDB = false;
sqlite3 *dbfile;

string id = "0";
string image = "";
string mode = "0";
string state = "";
string feedback = "";
int idle = 1;

bool ConnectDB ();
void DisonnectDB ();

void getImageToProcess();
int updateImage(int objectfound);
int skipImage();

void readme();
int skipFrames(VideoCapture capture, int num);
void gpuFindGoodMatches(vector< DMatch > matches, vector< DMatch > &good_matches);
void findH(vector<KeyPoint> keypoints_object, vector<KeyPoint> keypoints_scene, vector<DMatch> good_matches, Mat &H, vector<uchar> &inliers);
void getImageCorners(Mat img_object, vector<Point2f> &object_corners);
bool findRect(vector<Point2f> scene_corners, Rect &rect);
void drawRects(Mat img_results, vector<Point2f> object_corners, vector<Point2f> scene_corners, Mat img_object);
void anounceFeedback(string feedback);

int getdir (string dir, vector<string> &files) {
    DIR *dp;
    struct dirent *dirp;
    if((dp  = opendir(dir.c_str())) == NULL) {
        cout << "Error(" << errno << ") opening " << dir << endl;
        return errno;
    }
    
    while ((dirp = readdir(dp)) != NULL) {
        files.push_back(string(dirp->d_name));
    }
    closedir(dp);
    return 0;
}

bool hasEnding (std::string const &fullString, std::string const &ending) {
    if (fullString.length() >= ending.length()) {
        return (0 == fullString.compare (fullString.length() - ending.length(), ending.length(), ending));
    } else {
        return false;
    }
}

/** @function main */
int main(int argc, char** argv)
{
    cout << "Start (Facade Aiming dynamic)"<< endl;
    
    isOpenDB = ConnectDB();
    
    if (isOpenDB) cout << "Connected Successful" << endl;
    else cout << "connection failed " << endl;

    vector<Mat> img_object_vector = vector<Mat>();

    vector<GpuMat> keypoints_object_gpu_vector = vector<GpuMat>();
    vector<GpuMat> descriptors_object_gpu_vector = vector<GpuMat>();

    vector<vector<KeyPoint> > keypoints_object_vector = vector<vector<KeyPoint> >();
    vector<vector<float> > descriptors_object_vector = vector<vector<float> >();
    
    string dir = string(refImageDirBase) + string(session) + "/";
    vector<string> filenames = vector<string>();
    vector<string> files = vector<string>();
    
    getdir(dir,filenames);
    
    string ending = "jpg";

    SURF_GPU surf;
    surf.hessianThreshold = 400;

    cv::gpu::printShortCudaDeviceInfo(cv::gpu::getDevice());

    for (unsigned int i = 0;i < filenames.size();i++) {
        if (hasEnding (filenames[i], ending)) {
            cout <<  filenames[i] << endl;
            Mat img_object = imread(dir+filenames[i]);
            
            files.push_back(filenames[i]);
            
            double ratio = img_object.rows > img_object.cols ? 600.0 / (double)img_object.rows : 600.0 / (double) img_object.cols;
            
            // Size img_object_size(150, 445); //(150, 445)
            // resize(img_object, img_object, img_object_size);
            
            Size img_object_size(img_object.cols * ratio, img_object.rows * ratio); //(150, 445)
            resize(img_object, img_object, img_object_size);

            img_object_vector.push_back(img_object);
            
            cout << dir+filenames[i];
            
            // Computer SURF keypoints and descriptors for object (reference) image
            GpuMat img_object_gpu;
            Mat img_object_gray;
            cvtColor(img_object, img_object_gray, CV_RGB2GRAY);
            img_object_gpu.upload(img_object_gray);
            
            GpuMat keypoints_object_gpu;
            GpuMat descriptors_object_gpu;
            surf(img_object_gpu, GpuMat(), keypoints_object_gpu, descriptors_object_gpu);
            
            keypoints_object_gpu_vector.push_back(keypoints_object_gpu);
            descriptors_object_gpu_vector.push_back(descriptors_object_gpu);
            
            cout << "FOUND " << keypoints_object_gpu.cols << " keypoints on reference image" << endl;
            
            // downloading objects results
            vector<KeyPoint> tmp_keypoints_object;
            vector<float> tmp_descriptors_object;
            
            surf.downloadKeypoints(keypoints_object_gpu, tmp_keypoints_object);
            surf.downloadDescriptors(descriptors_object_gpu, tmp_descriptors_object);
            
            keypoints_object_vector.push_back(tmp_keypoints_object);
            descriptors_object_vector.push_back(tmp_descriptors_object);
        }
    }

    for (int i = 0; i < files.size(); ++i){
        cout << files[i] << endl;
    }

    GpuMat keypoints_scene_gpu;
    GpuMat descriptors_scene_gpu;

    int count = 0;
    clock_t last_time = clock();
    printf("Gran = %f\n", 0.05 * 1000000);
    
    int object_not_found = 1;
    int db_object_found = -1;
    
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

            // downloading scene results
            vector<KeyPoint> keypoints_scene;
            vector<float> descriptors_scene;
            surf.downloadKeypoints(keypoints_scene_gpu, keypoints_scene);
            surf.downloadDescriptors(descriptors_scene_gpu, descriptors_scene);
            
            vector< DMatch > good_matches;
            int state_index = -1;

            Mat H;
            int inlier_max = -1;
            
            for (int i = 0;i < files.size();i++) {
                // matching descriptors
                BFMatcher_GPU matcher(NORM_L2);
                GpuMat trainIdx, distance;
                matcher.matchSingle(descriptors_object_gpu_vector[i], descriptors_scene_gpu, trainIdx, distance);
                vector<DMatch> matches;
                BFMatcher_GPU::matchDownload(trainIdx, distance, matches);
                
                vector< DMatch > tmp_good_matches;
                gpuFindGoodMatches(matches, tmp_good_matches);

                // Find H
                Mat tempH;
                vector<uchar> inliers;
                findH(keypoints_object_vector[i], keypoints_scene, tmp_good_matches, tempH, inliers);

                // Only count non '\0' inliers
                int inliersCount = 0;
                for (int i = 0; i < inliers.size(); i++) {
                    if (inliers[i] != '\0') {
                        inliersCount++;
                    }
                }

                cout << "Inlier count: " << i << " " << inliersCount << endl;
                
                if (i == 0 || inliersCount > inlier_max) {
                    inlier_max = inliersCount;
                    state_index = i;
                    H = tempH;
                }
                
                // Mat img_results;
                /* drawMatches( img_object, keypoints_object, img_scene, keypoints_scene,
                 good_matches, img_results, Scalar::all(-1), Scalar::all(-1),
                 vector<char>(), DrawMatchesFlags::NOT_DRAW_SINGLE_POINTS ); */
            }
            
            string current_state = files[state_index].substr(0, files[state_index].size() - 4);
            cout << "Current state: " << current_state << endl;

            Mat img_object = img_object_vector[state_index];
            
            Mat img_results;
            /* drawMatches( img_object, keypoints_object, img_scene, keypoints_scene,
             good_matches, img_results, Scalar::all(-1), Scalar::all(-1),
             vector<char>(), DrawMatchesFlags::NOT_DRAW_SINGLE_POINTS ); */
            
            // Get the corners from the object (reference) image
            vector<Point2f> object_corners(5);
            getImageCorners(img_object, object_corners);
            
            // Transform object corners to scene corners using H
            vector<Point2f> scene_corners(5);
            perspectiveTransform( object_corners, scene_corners, H);

            Rect rect;
            if (!findRect(scene_corners, rect)) {
                db_object_found = 0;
                
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

                Point2f object_center_scene;
                object_center_scene.x = (scene_corners[0].x + scene_corners[1].x + scene_corners[2].x + scene_corners[3].x) / 4;
                object_center_scene.y = (scene_corners[0].y + scene_corners[1].y + scene_corners[2].y + scene_corners[3].y) / 4;

                cout << "Center: x: " << object_center_scene.x << " y: " << object_center_scene.y << endl;

                feedback = "";

                if ((scene_corners[0].y < 0 || scene_corners[1].y < 0 || 
                    scene_corners[2].y < 0 || scene_corners[3].y < 0) &&
                    (scene_corners[0].y > img_scene.rows || scene_corners[1].y > img_scene.rows || 
                    scene_corners[2].y > img_scene.rows || scene_corners[3].y > img_scene.rows)) {
                    feedback += "further ";
                } else if ((scene_corners[0].x < 0 || scene_corners[1].x < 0 || 
                    scene_corners[2].x < 0 || scene_corners[3].x < 0) &&
                    (scene_corners[0].y > img_scene.cols || scene_corners[1].y > img_scene.cols || 
                    scene_corners[2].y > img_scene.cols || scene_corners[3].y > img_scene.cols)){
                    feedback += "further ";
                } else {
                    if (scene_corners[0].y < 0 || scene_corners[1].y < 0 || 
                        scene_corners[2].y < 0 || scene_corners[3].y < 0) {
                        feedback += "right ";
                    } else if (scene_corners[0].y > img_scene.rows || scene_corners[1].y > img_scene.rows || 
                        scene_corners[2].y > img_scene.rows || scene_corners[3].y > img_scene.rows) {
                        feedback += "left ";

                    }

                    if (scene_corners[0].x < 0 || scene_corners[1].x < 0 || 
                        scene_corners[2].x < 0 || scene_corners[3].x < 0) {
                        feedback += "up ";
                    } else if (scene_corners[0].x > img_scene.cols || scene_corners[1].x > img_scene.cols || 
                        scene_corners[2].x > img_scene.cols || scene_corners[3].x > img_scene.cols) {
                        feedback += "down ";
                    }
                }

                // if (object_center_scene.y < 0) {
                //     feedback += "up ";
                // } else if (object_center_scene.y > img_scene.rows) {
                //     feedback += "down ";
                // }
                // if (object_center_scene.x < 0) {
                //     feedback += "right";
                // } else if (object_center_scene.x > img_scene.cols) {
                //     feedback += "left";
                // }

                if (feedback.compare("")) {
                    feedback = "Move phone to " + feedback;
                } else {
                    feedback = "Aiming is good";
                }

                if (state.compare(current_state)) {
                    feedback = "State: " + current_state;
                    state = current_state;
                }
            }
            
            //-- Show detected matches
            // imshow( "Overall Results", img_results );

            // anounceFeedback(feedback);
            
            cout << "Feedback: " << feedback << endl;
            
            clock_t toc = clock();
            printf("Elapsed: %f seconds\n", (double)(toc - tic) / CLOCKS_PER_SEC);
            /****** END CLOCK *******/
            
            updateImage(db_object_found);
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
    
    sprintf(query, "SELECT image, id, imageactive FROM images_video WHERE imageactive>0 AND id>%s ORDER BY id DESC", id.c_str());

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
            idle = 0;
        } else if (firstResult == 101) {
            // cout << "Nothing." << endl;
        }
        sqlite3_finalize(statement);
    }
}

int updateImage(int objectfound) {
    sqlite3_stmt *statement;
    int result;
    
    char query[1000];
    
    sprintf(query, "UPDATE images_video SET imageactive=0,"
            "feedback=\"%s\",feedbackactive=1,resulttime=current_timestamp, objectfound=%s WHERE id=%s",
            feedback.c_str(), to_string(objectfound).c_str(), id.c_str());
    
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

void anounceFeedback(string feedback) {
    // Anounce feedback string
    stringstream ss;
    ss << "say \"" << feedback << "\"";
    system(ss.str().c_str());
}
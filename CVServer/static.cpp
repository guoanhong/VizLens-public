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
#include <set>

#include "opencv2/core/core.hpp"
#include "opencv2/features2d/features2d.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/calib3d/calib3d.hpp"
#include "opencv2/nonfree/nonfree.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/video/background_segm.hpp"
//#include <opencv2/gpu/gpu.hpp>
//#include "opencv2/nonfree/gpu.hpp"
// #include "sqlite3.h"

// #include "lib/db_utils.hpp"
#include "lib/img_utils.hpp"
#include "lib/statemap_utils.hpp"
#include "lib/feedback_utils.hpp"
#include "lib/visualization.hpp"

// TODO: define envpaths
// #define PATH genenv("APP_PATH")
#define DB "/home/guoanhong/Desktop/ApplianceReader/WebServer/db/ApplianceReader.db"
#define refImageDirBase "/Users/judy/Desktop/VizLens/samples/images/"
#define imageDirBase "/Users/judy/Desktop/VizLens/samples/videos" // VizLens Test/images_video/"
#define session ""

using namespace cv;
using namespace std;
//using namespace cv::gpu;

const int MIN_FINGER_DEPTH = 20;
const int MAX_FINGER_ANGLE = 60;
const double PI = 3.1415926;

bool isOpenDB = false;
sqlite3 *dbfile;

string id = "0";
string image = "";
string mode = "0";
string target = "";
string state = "";
string feedback = "";
string guidance = "";
int idle = 1;

void readme();
int skipFrames(VideoCapture capture, int num);

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
    cout << "Start (dynamic display)"<< endl;
    
//    isOpenDB = ConnectDB();
//    
//    if (isOpenDB) cout << "Connected Successful" << endl;
//    else cout << "connection failed " << endl;
    
    vector<Mat> img_object_vector = vector<Mat>();
    vector<vector<vector<int> > > button_map_vector = vector<vector<vector<int> > >();
    vector<vector<string> > true_labels_vector = vector<vector<string> >();
    vector<vector<vector<double> > > button_center_vector = vector<vector<vector<double> > >();
    vector<double> avg_button_size_vector = vector<double>();
    
    vector<Mat> keypoints_object_gpu_vector = vector<Mat>();
    vector<Mat> descriptors_object_gpu_vector = vector<Mat>();
    
    vector<vector<KeyPoint> > keypoints_object_vector = vector<vector<KeyPoint> >();
    vector<vector<float> > descriptors_object_vector = vector<vector<float> >();
    
    
    
    /******************** Read Images ********************/

    // JUDY
    
    int interface_id = getInterfaceId();
    string interface_name = getInterfaceName(interface_id);
    
    // NOTE: in user testing, can either let user input/choose the current interface,
    // or detect the current interface and let the user confirm.
    
    // string dir = string(refImageDirBase) + string(session) + "/";
    string dir = string(refImageDirBase) + string(session) + "/" + interface_name + "/";
    
    vector<string> filenames = vector<string>();
    vector<string> files = vector<string>();
    
    vector<int> states = vector<int>();
    
    vector<set<int>> transition_graph = vector<set<int>>();
    vector<set<int>> reachable_states = vector<set<int>>();
    
    vector<int> all_ids = vector<int>();

    // /* Initialize screenmap visualization */
    // Mat state_graph_pic = Mat::zeros( 250, 750, CV_8UC3 );
    
    getdir(dir,filenames);
    sort(filenames.begin(), filenames.end());
    // for (int i = 0; i < filenames.size(); i++) {
    //     cout << filenames[i] << "\n";
    // }
    
    getTransitionGraph(interface_id, transition_graph);
    initializeObjects(transition_graph, reachable_states);

    // /* Initial visualization preparation */
    // drawInitGraph(state_graph_pic, transition_graph);
    
    string ending = "jpeg";
    
    int prev_state_id = -1;
    int curr_state_id = -1;

    // NEEDED
//    SURF surf;
//    surf.hessianThreshold = 400;
    int minHessian = 400;
    SurfFeatureDetector detector(minHessian);
    
    for (unsigned int i = 0;i < filenames.size();i++) {
        if (hasEnding (filenames[i], ending)) {
            cout <<  filenames[i] << endl;
            Mat img_object = imread(dir+filenames[i]);
            
            files.push_back(filenames[i]);
            
            double ratio = img_object.rows > img_object.cols ? 600.0 / (double)img_object.rows : 600.0 / (double) img_object.cols;
            
            Size img_object_size(img_object.cols * ratio, img_object.rows * ratio); //(150, 445)
            resize(img_object, img_object, img_object_size);

            img_object_vector.push_back(img_object);
            
            vector<vector<int> > button_map;
            vector<string> true_labels;
            vector<vector<double> > button_center;
            double avg_button_size;
            labels_hash(filenames[i].substr(0, filenames[i].size()-9), img_object, ratio, button_map, true_labels, button_center, avg_button_size);
            
            button_map_vector.push_back(button_map);
            true_labels_vector.push_back(true_labels);
            button_center_vector.push_back(button_center);
            avg_button_size_vector.push_back(avg_button_size);
            
            cout << dir+filenames[i];
            lightingDetection(img_object);
            
            // Computer SURF keypoints and descriptors for object (reference) image
            Mat img_object_gpu;
            Mat img_object_gray;
            cvtColor(img_object, img_object_gray, CV_RGB2GRAY);
//            img_object_gpu.upload(img_object_gray);
//            cv::gpu::printShortCudaDeviceInfo(cv::gpu::getDevice());
            img_object_gpu = img_object;
            
            Mat keypoints_object_gpu;
            Mat descriptors_object_gpu;
            // ORIGINAL
            // surf(img_object_gpu, Mat(), keypoints_object_gpu, descriptors_object_gpu);
            
            // CPU ONLY
            vector<KeyPoint> keypoints_object;
            Mat descriptors_object; // SHOULD BE vector<float>
            int minHessian = 400;
            SurfFeatureDetector detector(minHessian);
            detector.detect(img_object_gpu, keypoints_object);
            printf("-- Keypoints Vector Size : %lu \n", keypoints_object.size() );
            //-- Step 2: Calculate reference descriptors (feature vectors)
            SurfDescriptorExtractor extractor;
            extractor.compute(img_object_gpu, keypoints_object, descriptors_object);
            
//            keypoints_object_gpu_vector.push_back(keypoints_object_gpu);
//            descriptors_object_gpu_vector.push_back(descriptors_object_gpu);
//            
//            cout << "FOUND " << keypoints_object_gpu.cols << " keypoints on first image" << endl;
//            
//            // downloading objects results
//            vector<KeyPoint> tmp_keypoints_object;
//            vector<float> tmp_descriptors_object;
            
            // ORIGINAL
            // surf.downloadKeypoints(keypoints_object_gpu, tmp_keypoints_object);
            // surf.downloadDescriptors(descriptors_object_gpu, tmp_descriptors_object);
            
            all_ids.push_back(i);
            
            keypoints_object_vector.push_back(keypoints_object);
            descriptors_object_gpu_vector.push_back(descriptors_object);
        }
    }

    for (int i = 0; i < files.size(); ++i){
        cout << files[i] << endl;
    }
    
    Mat keypoints_scene_gpu;
    Mat descriptors_scene_gpu;
    
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

    /******************** Read Scenes ********************/
    
    
    // CPU ONLY
//    VideoCapture capture("/Users/judy/Desktop/VizLens/VizLens Test/samples/iPhone_Microwave_Motion_Bright.mov");
//    VideoCapture capture("/Users/judy/Desktop/VizLens/VizLens Test/samples/Untitled.mov");
//    VideoCapture capture("/Users/judy/Desktop/VizLens/VizLens Test/samples/kiosk.mov");
//    VideoCapture capture("/Users/judy/Desktop/VizLens/VizLens Test/samples/kiosk_fast.mov");
//    VideoCapture capture("/Users/judy/Desktop/VizLens/VizLens Test/samples/checkout.mov");
    // VideoCapture capture("/Users/judy/Desktop/VizLens/VizLens Test/samples/other.mov");
    VideoCapture capture("/Users/judy/Desktop/VizLens/samples/videos/DynamicTest_images_video.m4v");
//    capture.open(0); // Capture from camera
    if (!capture.isOpened()) {
        throw "--(!) Error reading steam";
    }
    int frame = 0;
    int first_skip_frames = 0;
    for (int i = 0; i < first_skip_frames; i++) { capture.grab(); }
    frame += first_skip_frames;
    
    int i;
    for ( ; ; ) {
        try {
            // ORIGINAL
//            while (idle == 1) {
//                if(clock() - last_time > (double)(0.05 * 1000000)) {
//                    count++;
//                    getImageToProcess();
//                    last_time = clock();
//                }
//            }
//            
//            Mat img_scene = imread(imageDirBase+image);
            
//            // TODO: Add get image
//            Mat img_scene;
//            if(!capture.read(img_scene))
//                imshow("",img_scene);
//
            
            ++i;
            
            // CPU ONLY
            Mat img_scene;
            int frames_skipped = 5;
            for (int a = 0;  a < frames_skipped - 1; a++) {
                capture.grab();
            }
            capture >> img_scene;
            if (img_scene.empty()) break;
            // Output scenes for debugging
            
            double scene_ratio = img_scene.rows > (double)img_scene.cols / 3 * 2 ? 500 / (double)img_scene.rows : 750 / (double)img_scene.cols;
            // img_scene.rows > img_scene.cols ? 600.0 / (double)img_scene.rows : 600.0 / (double) img_scene.cols;
            Size img_scene_size(img_scene.cols * scene_ratio, img_scene.rows * scene_ratio); //(150, 445)
            resize(img_scene, img_scene, img_scene_size);
            
            // imshow("", img_scene);
            
            frame += frames_skipped;
            
            
            
            // Size img_scene_size(360, 640);
            
            if (img_scene.empty()) break;
            
            cout << "--Size of frame is: "<< img_scene.cols << " " << img_scene.rows << endl;
            
            lightingDetection(img_scene);
            
            /****** START CLOCK *******/
            clock_t tic = clock();
            
            // Computer SURF keypoints and descriptors for scene (video) image
            Mat img_scene_gpu;
            Mat img_scene_gray;
            cvtColor(img_scene, img_scene_gray, CV_RGB2GRAY);
            // NEEDED
            // img_scene_gpu.upload(img_scene_gray);
            // surf(img_scene_gpu, Mat(), keypoints_scene_gpu, descriptors_scene_gpu);
            img_scene_gpu = img_scene;
            
            // CPU ONLY
            vector<KeyPoint> keypoints_scene;
            Mat descriptors_scene; // SHOULD BE vector<float>
            detector.detect(img_scene_gpu, keypoints_scene);
            printf("-- Keypoints Vector Size : %lu \n", keypoints_scene.size() );
            //-- Step 2: Calculate reference descriptors (feature vectors)
            SurfDescriptorExtractor extractor;
            extractor.compute(img_scene_gpu, keypoints_scene, descriptors_scene);
            
//            cout << "FOUND " << keypoints_scene_gpu.cols << " keypoints on second image" << endl;
//            
//            if (keypoints_scene_gpu.cols == 0) {
//                idle = 1;
//                skipImage();
//                continue;
//            }
            
//            // downloading scene results
//            vector<KeyPoint> keypoints_scene;
//            vector<float> descriptors_scene;
//            // NEEDED
            // surf.downloadKeypoints(keypoints_scene_gpu, keypoints_scene);
            // surf.downloadDescriptors(descriptors_scene_gpu, descriptors_scene);
            
            /******************** Start Matching ********************/

            vector< DMatch > good_matches;
            int state_index = -1;

            Mat H;
            int inlier_max = -1;
            
            // THE ORIGINAL MATCHING PROCESS
            // for (int i = 0;i < files.size();i++) {
            // ....
            // }

            // NEW MATCHING WITH THE "STEP" FILTER
            vector<Mat> reachable_descriptors_object_gpu_vector;
            if (prev_state_id >= 0) {
                reachable_descriptors_object_gpu_vector = vector<Mat>();
                filterObjects(prev_state_id, reachable_states, reachable_descriptors_object_gpu_vector, descriptors_object_gpu_vector);
            } else {
                reachable_descriptors_object_gpu_vector = descriptors_object_gpu_vector;
            }
            
            vector<Mat> reachable_img_object_vector;
            if (prev_state_id >= 0) {
                reachable_img_object_vector = vector<Mat>();
                filterOriginalImgs(prev_state_id, reachable_states, reachable_img_object_vector, img_object_vector);
            } else {
                reachable_img_object_vector = img_object_vector;
            }
            
            vector<int> reachable_ids = all_ids;
            if (prev_state_id >= 0) {
                reachable_ids.resize(reachable_states[prev_state_id].size());
                copy(reachable_states[prev_state_id].begin(), reachable_states[prev_state_id].end(), reachable_ids.begin());
            }
            
            for (int i = 0;i < reachable_descriptors_object_gpu_vector.size();i++) {
                // matching descriptors
                BFMatcher matcher(NORM_L2);
                Mat trainIdx, distance;
                
                // CPU ONLY
                vector<DMatch> matches;
                matcher.match(reachable_descriptors_object_gpu_vector[i], descriptors_scene, matches);
                
                vector< DMatch > tmp_good_matches;
                gpuFindGoodMatches(matches, tmp_good_matches);

                // DEBUG
                if (tmp_good_matches.size() < 5) {
                    continue;
                }
                
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
                


            }
            
            
            
            // Use the history of detected screens to determine the current screen and get rid of noises.
            // By JUDY
            if (states.size() == 6) {
                states.erase(states.begin());
            }
            states.push_back(state_index);
            // states.push_back(matched_state);

            prev_state_id = curr_state_id;
            curr_state_id = findCurrentState(states);
            string matched_state = files[state_index].substr(0, files[state_index].size() - 9);
            string current_state = files[curr_state_id].substr(0, files[curr_state_id].size() - 9);
            
            cout << "CURRENT STATE: " << current_state << endl;
            
            


            /******************** Send Feedback ********************/


            // NEED TO FIX state_index

            Mat img_object = img_object_vector[state_index];
            vector<vector<int> > button_map = button_map_vector[state_index];
            vector<string> true_labels = true_labels_vector[state_index];
            vector<vector<double> > button_center = button_center_vector[state_index];
            double avg_button_size = avg_button_size_vector[state_index];

//            double object_ratio = img_scene.rows > img_object.cols ? 200.0 / (double)img_object.rows : 200.0 / (double) img_object.cols;
//            Size img_object_size(img_object.cols * object_ratio, img_object.rows * object_ratio); //(150, 445)
//            resize(img_object, img_object, img_object_size);
            
            // DRAW MAT
            Mat img_results = Mat::zeros( 750, 1200, CV_8UC3 );
            drawScreenMap(img_results, img_scene, reachable_img_object_vector, curr_state_id, reachable_ids);
            drawStateGraph(img_results, transition_graph, curr_state_id);

            displayLogOnScreen(img_results, interface_name, files, states, matched_state, current_state);

//            img_object.copyTo(img_results(Rect(0,0,img_object.cols, img_object.rows)));
//            img_scene.copyTo(img_results(Rect(img_object.cols,0,img_scene.cols, img_scene.rows)));
            
            
//            drawMatches( img_object, keypoints_object_vector[state_index], img_scene, keypoints_scene_gpu,
//                        good_matches, img_results, Scalar::all(-1), Scalar::all(-1),
//                        vector<char>(), DrawMatchesFlags::NOT_DRAW_SINGLE_POINTS );
            
//            Mat img_results;
//            imshow("",img_scene);
//            imshow("", img_object);
            
            // Get the corners from the object (reference) image
            vector<Point2f> object_corners(5);
            getImageCorners(img_object, object_corners);
            
            // Transform object corners to scene corners using H
            vector<Point2f> scene_corners(5);
            perspectiveTransform( object_corners, scene_corners, H);
//            cout << "Image corners: " << scene_corners[1] << scene_corners[2] << scene_corners[3] << scene_corners[4] << endl;
            

            /* Not Needed for Viz Dynamic

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
//                        feedback = "no object";
                        object_not_found++;
                    } else {
                        feedback = "";
                        object_not_found++;
                    }
                    
//                    cout << "No Object" << endl ;
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
                    cout << "-- Fingertip Location:" << fingertip_location << scene_fingertip_location << endl;
                    
                    if (state.compare(current_state)) {
                        feedback = "State: " + current_state + " " + feedback;
                        state = current_state;
                    }
                    
                }
            }
            
            */ 

            imshow( "Overall Results", img_results );

            anounceFeedback(feedback);
            
            cout << "Feedback: " << feedback << endl;
            
            clock_t toc = clock();
            printf("Elapsed: %f seconds\n", (double)(toc - tic) / CLOCKS_PER_SEC);
            

            /****** END CLOCK *******/

            


            // updateImage(db_object_found, db_finger_found, fingerx, fingery); // To be added when sqlite is back
            idle = 1;
            
            waitKey(1); // waits to display frame
        } catch (Exception &e) {
            idle = 1;
            // skipImage(); // To be added when sqlite is back
            continue;
        }
    }
    
    waitKey(0);
    return 0;
}




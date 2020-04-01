#include <iostream>
#include "curl/curl.h"
#include "opencv2/nonfree/nonfree.hpp"
//#include <opencv2/gpu/gpu.hpp>
//#include "opencv2/nonfree/gpu.hpp"
#include "sqlite3.h"

#include "lib/db_utils.hpp"
#include "lib/img_utils.hpp"
#include "lib/ocr_utils.hpp"
#include "lib/statemap_utils.hpp"
#include "lib/feedback_utils.hpp"
#include "lib/visualization.hpp"

#define DB "../WebServer/db/VizLensDynamic_Building.db"
#define imageDirBase "../WebServer/images_video/"
#define dirBase "./samples/"

const string session_name = "coffee_machine";
const string read_mode = "db";

using namespace cv;
using namespace std;
//using namespace cv::gpu;
// using namespace tesseract;

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
int curr_state_id = -1;
Mat curr_state_img_scene;
float benchmark_ratio = 1.0; // Disable benchmark ratio for now

void readme();
int skipFrames(VideoCapture capture, int num);

/* hasEnding: Check for correct file format */
bool hasEnding (std::string const &fullString, std::string const &ending) {
    if (fullString.length() >= ending.length()) {
        return (0 == fullString.compare (fullString.length() - ending.length(), ending.length(), ending));
    } else {
        return false;
    }
}

/* NotAlphaNumerical: Filter out special characters encounterd in OCR */
/*
bool NotAlphaNumerical(char c) {
    return !(('0' <= c && c <= '9') || ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z') 
        || c == ' ' || c == '\n');
}
*/

/** @function main */
int main(int argc, char** argv)
{

    cout << "Start (dynamic display)"<< endl;

    string folder = string(argv[1]);
    string video = string(argv[2]); // Example: "video_coffee_hard.MOV";

    // METADATA: Thresholds
    float small_diff_ratio = 0.05; // If diff of current state and detected state is below this value, consider it a noise
    float match_expectation_ratio = 0.8; // If ratio of expected state is above this value, consider it a match

    float nonmatched_ratio_threshold = atof(argv[3]); // 0.3; // Below this value, consider it as a new state
    float matched_ratio_threshold = atof(argv[4]); // 0.5; // Above this value, consider it as a match
    float neighbor_ratio_threshold = matched_ratio_threshold; // If max ratio among neighbors is above this value, consider it a match
    // This value is set higher than that in access mode to avoid missing new screens

    float ocr_threshold_upper = 0.7; // Edit distance / length of short text must be above this value to be considered a match
    float ocr_threshold_lower = 0.6; // Edit distance / length of long text must be above this value to be considered a match

    bool enableCropping = stoi(argv[5]);
    bool enableOCR = stoi(argv[6]);

    int frames_skipped = 4;
    int state_transition_smoothing = 2;
    
    /* Initialize vectors to store reference images and buttons */
    vector<Mat> img_object_vector = vector<Mat>();
    vector<vector<int>> button_id_vector = vector<vector<int>>();
    vector<vector<vector<int> > > button_map_vector = vector<vector<vector<int> > >();
    vector<vector<string> > true_labels_vector = vector<vector<string> >();
    vector<vector<vector<double> > > button_center_vector = vector<vector<vector<double> > >();
    vector<double> avg_button_size_vector = vector<double>();
    vector<Mat> keypoints_object_gpu_vector = vector<Mat>();
    vector<Mat> descriptors_object_gpu_vector = vector<Mat>();
    vector<vector<KeyPoint> > keypoints_object_vector = vector<vector<KeyPoint> >();
    vector<vector<float> > descriptors_object_vector = vector<vector<float> >();
    vector<string> ocr_object_vector = vector<string>();

    /* Initialize vectors to store objects and transition graph */
    vector<string> objectnames = vector<string>();
    vector<string> filenames = vector<string>();
    vector<string> files = vector<string>();
    vector<string> description_text_vector = vector<string>();
    vector<map<int, int>> transition_graph = vector<map<int, int>>();
    // vector<set<int>> reachable_states = vector<set<int>>();
    vector<int> all_ids = vector<int>();
    vector<int> detected_states = vector<int>(); // Pool of detected states for smoothing
    vector<Point2f> detected_fingertip_locations = vector<Point2f>(); // Pool of fingertop locations mapped with ids

    /* Initialize vectors to store candidates of new reference images */
    vector<Mat> new_img_pool;
    vector<vector<KeyPoint> > new_keypoints_pool = vector<vector<KeyPoint> >();
    vector<Mat> new_descriptors_pool = vector<Mat>();
    // vector<string> new_ocr_vector = vector<string>();

    /******************** DB Connection ********************/
    isOpenDB = ConnectDB(DB);
   
    if (isOpenDB) cout << "Connected Successful" << endl;
    else cout << "connection failed " << endl;

    /**** Set up curl request ****/
    // CURL *hnd = curl_easy_init();
    // curl_easy_setopt(hnd, CURLOPT_CUSTOMREQUEST, "POST");
    // curl_easy_setopt(hnd, CURLOPT_URL, "http://localhost:8888/VizLensDynamic/php/uploadOriginal.php");

    // struct curl_slist *headers = NULL;
    // headers = curl_slist_append(headers, "cache-control: no-cache");
    // headers = curl_slist_append(headers, "content-type: multipart/form-data; boundary=----WebKitFormBoundary7MA4YWxkTrZu0gW");
    // curl_easy_setopt(hnd, CURLOPT_HTTPHEADER, headers);

    /******************** Read Reference Images (objects) ********************/     
    string dir = string(dirBase) + folder + "/images/";
    string dir_server = "./../WebServer/images_reference/" + session_name + "/";
    getRefImageFilenames(session_name, dir, filenames, read_mode);
    getTransitionGraph(session_name, read_mode, filenames, transition_graph);
    
    /* Load object images from WebServer image_reference folder */
    string ending = "jpg";

    /* GPU code */
    /*
    SURF surf;
    surf.hessianThreshold = 400;
    */
    int minHessian = 400;
    SurfFeatureDetector detector(minHessian);
    
    int id_count = 0;
    for (unsigned int i = 0;i < filenames.size();i++) {
        if (hasEnding (filenames[i], ending)) {
            id_count++;
            cout <<  filenames[i] << endl;
            Mat img_object = imread(dir_server+filenames[i]);
            cout << dir_server+filenames[i] << endl;            
            files.push_back(filenames[i]);
            
            double ratio = img_object.rows > img_object.cols ? 600.0 / (double)img_object.rows : 600.0 / (double) img_object.cols;

            Size img_object_size(img_object.cols * ratio, img_object.rows * ratio); //(150, 445)
            resize(img_object, img_object, img_object_size);

            img_object_vector.push_back(img_object);
            
            vector<int> button_id;
            vector<vector<int> > button_map;
            vector<string> true_labels;
            vector<vector<double> > button_center;
            double avg_button_size;
            labels_hash(session_name, id_count - 1, img_object, ratio, button_id, button_map, true_labels, button_center, avg_button_size);
            // labels_hash(filenames[i].substr(0, filenames[i].size()- 4), img_object, ratio, button_map, true_labels, button_center, avg_button_size);

            button_id_vector.push_back(button_id);
            button_map_vector.push_back(button_map);
            true_labels_vector.push_back(true_labels);
            button_center_vector.push_back(button_center);
            avg_button_size_vector.push_back(avg_button_size);
            
            cout << dir+filenames[i] << endl;
            lightingDetection(img_object);
            
            /* Computer SURF keypoints and descriptors for object (reference) image */
            Mat img_object_gpu;
            Mat img_object_gray;
            cvtColor(img_object, img_object_gray, CV_RGB2GRAY);
            /* GPU code */
            /*
            img_object_gpu.upload(img_object_gray);
            cv::gpu::printShortCudaDeviceInfo(cv::gpu::getDevice());
            */
            img_object_gpu = img_object;
            
            Mat keypoints_object_gpu;
            Mat descriptors_object_gpu;
            /* GPU code */
            // surf(img_object_gpu, Mat(), keypoints_object_gpu, descriptors_object_gpu);
            
            /* CPU code */
            vector<KeyPoint> keypoints_object;
            Mat descriptors_object; // GPU: vector<float>
            int minHessian = 400;
            SurfFeatureDetector detector(minHessian);
            detector.detect(img_object_gpu, keypoints_object);
            printf("-- Keypoints Vector Size : %lu \n", keypoints_object.size() );
            /* Calculate reference descriptors (feature vectors) */
            SurfDescriptorExtractor extractor;
            extractor.compute(img_object_gpu, keypoints_object, descriptors_object);
            
            /* GPU code */
            /*
            keypoints_object_gpu_vector.push_back(keypoints_object_gpu);
            descriptors_object_gpu_vector.push_back(descriptors_object_gpu);
            
            cout << "FOUND " << keypoints_object_gpu.cols << " keypoints on first image" << endl;
            
            // Download objects results
            vector<KeyPoint> tmp_keypoints_object;
            vector<float> tmp_descriptors_object;            
            surf.downloadKeypoints(keypoints_object_gpu, tmp_keypoints_object);
            surf.downloadDescriptors(descriptors_object_gpu, tmp_descriptors_object);
            */

            all_ids.push_back(id_count - 1);
            objectnames.push_back(filenames[i].substr(0, filenames[i].size() - 4));
            keypoints_object_vector.push_back(keypoints_object);
            descriptors_object_gpu_vector.push_back(descriptors_object);
        }
    }

    // TODO(judy): Need to DEBUG this part
    if (hasReadyToReadObjects()) {
        int new_state;
        string new_name;
        Mat new_object;
        updateCroppedImage(session_name, dir_server, new_name, new_state, new_object);
                            
        // Computer SURF keypoints and descriptors for object (reference) image
        Mat new_object_gpu;
        Mat new_object_gray;
        cvtColor(new_object, new_object_gray, CV_RGB2GRAY);
        // GPU code
        // img_object_gpu.upload(img_object_gray);
        // cv::gpu::printShortCudaDeviceInfo(cv::gpu::getDevice());
        new_object_gpu = new_object;
          
        Mat new_keypoints_object_gpu;
        Mat new_descriptors_object_gpu;
        // GPU code
        // surf(img_object_gpu, Mat(), keypoints_object_gpu, descriptors_object_gpu);
        
        // CPU code
        vector<KeyPoint> new_keypoints_object;
        Mat new_descriptors_object; // GPU: vector<float>
        int minHessian = 400;
        SurfFeatureDetector detector(minHessian);
        detector.detect(new_object_gpu, new_keypoints_object);
        printf("-- Keypoints Vector Size : %lu \n", new_keypoints_object.size() );
                
        // Calculate reference descriptors (feature vectors)
        SurfDescriptorExtractor extractor;
        extractor.compute(new_object_gpu, new_keypoints_object, new_descriptors_object);

        all_ids.push_back(new_state);
        objectnames.push_back(new_name);
        keypoints_object_vector.push_back(new_keypoints_object);
        descriptors_object_gpu_vector.push_back(new_descriptors_object);

        // VizDynamic: May need crowdsourcing for labeling buttons in added image object
        vector<int> button_id;
        vector<vector<int> > button_map;
        vector<string> true_labels;
        vector<vector<double> > button_center;
        double avg_button_size;
        double obj_ratio = new_object.rows > new_object.cols ? 600.0 / (double)new_object.rows : 600.0 / (double) new_object.cols;
        Size object_size(new_object.cols * obj_ratio, new_object.rows * obj_ratio); //(150, 445)
        resize(new_object, new_object, object_size);
        labels_hash(session_name, new_state, new_object, obj_ratio, button_id, button_map, true_labels, button_center, avg_button_size);
            
        button_map_vector.push_back(button_map);
        true_labels_vector.push_back(true_labels);
        button_center_vector.push_back(button_center);
        avg_button_size_vector.push_back(avg_button_size);
    }

    for (int i = 0; i < all_ids.size(); i++) {
        description_text_vector.push_back(getStateDescription(session_name, i));
        cout << "Description Text: " << description_text_vector[i] << endl;
    }

    /* Initialize OCR */
    // TODO(judy): This is temporary when we don't have OCR stored in DB. Once we have OCR always stored in DB, remove this part.
    for (int i = 0; i < img_object_vector.size(); i++) {
        if (enableOCR) {
            Mat img_object = img_object_vector[i];
            String text_object;
            getOCRTextFromImage(img_object, text_object);
            ocr_object_vector.push_back(text_object);
            cout << "OCR for ref image " << i << ": " << text_object << endl;
        }
    }

    /******************** Read Video (scenes) and Match with Reference Images (objects) ********************/
    
    /* Initialize values of detected screen and fintertip location*/
    Mat keypoints_scene_gpu;
    Mat descriptors_scene_gpu;
    
    int count = 0;
    clock_t last_time = clock();
    
    string previous_guidance = "";
    int prev_state_id = -1; // curr_state_id = -1;
    
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
    
    /* CPU code */
    string video_name = string(dirBase) + folder + "/videos/" + video;
    VideoCapture capture(video_name);
    if (!capture.isOpened()) {
        throw "--(!) Error reading stream";
    }
    int frame = 0;
    int first_skip_frames = 0;
    for (int i = 0; i < first_skip_frames; i++) { capture.grab(); }
    frame += first_skip_frames;
    
    Mat H;
    int i;
    id = "0";

    /* Start capturing scenes and matching with reference images */
    bool enableAddNewRefImage = true;
    for ( ; ; ) {
        try {

            /* Read scene */

            // Scene doesn't read in valid image
            // Update cropped image doesn't change it to read

            // Note: Need to add back for video streaming
            /*
            string image_to_process;
            // ORIGINAL
            while (idle == 1) {
                if(clock() - last_time > (double)(0.05 * 1000000)) {
                    count++;
                    getImageToProcess(image_to_process);
                    last_time = clock();
                }
            }
            // int frames_skipped = 4;
            Mat img_scene = imread(imageDirBase + session_name + "/" + image_to_process);
            cout << imageDirBase + session_name + "/" + image_to_process << endl;
            cout << "reading img_scene" << endl;
            */
            
            ++i;
            
            // CPU code
            /* Use local videos for image stream */
            Mat img_scene;
            Mat img_scene_raw;
            String img_text;
            for (int a = 0;  a < frames_skipped - 1; a++) {
                capture.grab();
            }
            capture >> img_scene;
            if (img_scene.empty()) break;
            /* Use local videos for image stream */
            
            if (img_scene.empty()) {
                idle = 1;
                skipImage();
                continue;
            }

            // frame += frames_skipped;

            /* lighting -- 
            lightingDetection(img_scene);
            */

            // isStateProcessing(0);

            /****** START CLOCK *******/
            clock_t tic = clock();
            clock_t toc = clock();

            /****** FOREGROUND & BACKGROUND DETECTION, CROPPING ******/
            img_scene_raw = img_scene;
            if (enableCropping) {
                string screen_exist;
                vector<double> screen_box{-1, -1, -1, -1};
                getScreenRegionFromImage(img_scene, screen_exist, screen_box);
                // Transform from proportion to pixel, keeping -1 indicator
                int screen_top = screen_box[0] == -1 ? -1 : screen_box[0] * img_scene.rows;
                int screen_left = screen_box[0] == -1 ? -1 : screen_box[1] * img_scene.cols;
                int screen_width = screen_box[0] == -1 ? -1 : screen_box[2] * img_scene.cols;
                int screen_height = screen_box[0] == -1 ? -1 : screen_box[3] * img_scene.rows;
                cout << "Screen detection " << i << ": " << screen_exist << endl;
                cout << "Screen region " << i << ": " << endl;
                cout<< "  Top:   " << screen_top << endl;
                cout<< "  Left:  " << screen_left << endl;
                cout<< "  Width: " << screen_width << endl;
                cout<< "  Height:" << screen_height << endl;

                if (screen_exist == "0") {
                    // Case 1: Not detected screen
                    // Directly skip the current scene
                    // imshow("img_scene", img_scene);
                    cout << "---- SCREEN NOT DETECTED! SKIPPING ----" << endl;
                    continue;
                } else if (screen_box[0] >= 0) {
                    // Case 2: Detected screen, with bounding box
                    // Crop the image, use only detected screen for matching
                    cout << "---- CROPPING ----" << endl;
                    img_scene = img_scene(Rect(screen_left, screen_top, screen_width, screen_height));
                } else { 
                    // Case 3: Detected screen, without bounding box
                    // Do nothing, use raw scene for matching
                    cout << "---- USING RAW SCENE ----" << endl;
                }
            }
            
            /* Computer SURF keypoints and descriptors for scene (video) image */
            Mat img_scene_gpu;
            Mat img_scene_gray;
            cvtColor(img_scene, img_scene_gray, CV_RGB2GRAY);
            /* GPU code */
            /*
            img_scene_gpu.upload(img_scene_gray);
            surf(img_scene_gpu, Mat(), keypoints_scene_gpu, descriptors_scene_gpu);
            */
            img_scene_gpu = img_scene_gray;
            /* CPU code */
            vector<KeyPoint> keypoints_scene;
            Mat descriptors_scene; // SHOULD BE vector<float>
            detector.detect(img_scene_gpu, keypoints_scene);
            SurfDescriptorExtractor extractor;
            extractor.compute(img_scene_gpu, keypoints_scene, descriptors_scene);
            
            cout << "FOUND " << keypoints_scene.size() << " keypoints on second image" << endl;
            // cout << "FOUND " << keypoints_scene_gpu.cols << " keypoints on second image" << endl;
            if (keypoints_scene.size() == 0) {
                idle = 1;
                skipImage();
                continue;
            }
            
            /* GPU code: */
            /*
            vector<KeyPoint> keypoints_scene;
            vector<float> descriptors_scene;
            surf.downloadKeypoints(keypoints_scene_gpu, keypoints_scene);
            surf.downloadDescriptors(descriptors_scene_gpu, descriptors_scene);
            */

            // For now, do not call OCR on every scene
            // Might test with OCR-only metric in technical evaluation
            /*
            string text_scene;
            getOCRTextFromImage(img_scene, text_scene);
            cout << "Detected text: " << text_scene << endl;
            */

            /******************** Start Matching ********************/

            // double lighting_scene = calculateLighting(img_scene);

            // FINGERTIP LOCATION
            Mat img_object;
            vector<vector<int> > button_map;
            vector<vector<double> > button_center;
            vector<string> true_labels;
            double avg_button_size;
            vector<Point2f> object_corners(5);
            vector<Point2f> scene_corners(5);
            vector<Point2f> fingertip_location(1);
            vector<Point2f> scene_fingertip_location(1);
            int button_index;
            int expected_state_id = -1;

            bool confirmed_curr_state = false;

            cout << "--------------------------------------------" << endl;
            cout << "---- Ratio Benchmark: " << benchmark_ratio << endl;
            // cout << "---- Lighting Stats: " << lighting_scene << endl;

            int state_index = 0;
            float ratio_max = 0;

            // Reduce search space: See if the scene stays on the same image
            // Reduce search space: See if the scene is the expected image
            // if (curr_state_id >= 0 && matchesCurrentState(descriptors_scene, keypoints_scene, curr_state_img_scene, descriptors_object_gpu_vector[curr_state_id], keypoints_object_vector[curr_state_id], benchmark_ratio, match_expectation_ratio)) {
            //     confirmed_curr_state = true;
            //     state_index = curr_state_id;
            //     cout << "---- Successfully matched with CURRENT state" << endl;
            // } else 
            if (expected_state_id >= 0 && matchesExpectedState(descriptors_scene, keypoints_scene, descriptors_object_gpu_vector[expected_state_id], keypoints_object_vector[expected_state_id], benchmark_ratio, match_expectation_ratio)) {
                confirmed_curr_state = true;
                state_index = expected_state_id;
                cout << "---- Successfully matched with EXPECTED state" << endl;
            } else {

                int inlier_max = -1;
            
                vector< DMatch > good_matches;
                vector<int> candidate_ids_object_vector;
                vector<Mat> candidate_descriptors_object_vector;
                
                getCandidateObjectIds(curr_state_id, all_ids, transition_graph, candidate_ids_object_vector);
                getCandidateObjectDescriptors(candidate_ids_object_vector, descriptors_object_gpu_vector, candidate_descriptors_object_vector);
                
                for (int i = 0; i < candidate_ids_object_vector.size(); i++) {
                    cout << "---- NEIGHBOR OF CURRENT STATE : " << candidate_ids_object_vector[i] << endl;
                }
                    
                cout << "------------------------------------------\n";
                cout << "Start Matching \n";
                cout << "------------------------------------------\n";

                float curr_state_ratio = 0.0;
                float expected_state_ratio = 0.0;
                for (int i = 0; i < candidate_descriptors_object_vector.size();i++) {
                    
                    // matching descriptors
                    BFMatcher matcher(NORM_L2);
                    Mat trainIdx, distance;
                    
                    /* CPU code */
                    vector<DMatch> matches;
                    matcher.match(candidate_descriptors_object_vector[i], descriptors_scene, matches);
                    // matcher.match(descriptors_object_gpu_vector[i], descriptors_scene, matches);

                    vector< DMatch > tmp_good_matches;
                    gpuFindGoodMatches(matches, tmp_good_matches);

                    // Can't find any object that matches the current scene
                    if (tmp_good_matches.size() < 4) {
                        cout << "-- Skipping Image -- " << i << endl;
                        continue;
                    }
                    // Find H
                    Mat tempH;
                    vector<uchar> inliers;
                    findH(keypoints_object_vector[candidate_ids_object_vector[i]], keypoints_scene, tmp_good_matches, tempH, inliers);
                    // Only count non '\0' inliers 
                    int inliersCount = 0;
                    for (int i = 0; i < inliers.size(); i++) {
                        if (inliers[i] != '\0') {
                            inliersCount++;
                        }
                    }

                    cout << "-- Match with Image -- " << candidate_ids_object_vector[i] << endl;
                    // cout << "OCR text: " << ocr_object_vector[i] << endl;
                    cout << "Object keypoints: " << keypoints_object_vector[i].size() << endl;
                    cout << "Scene keypoints: " << keypoints_scene.size() << endl;
                    cout << "Good matches: " << tmp_good_matches.size() << endl;
                    cout << "Inlier count: " << inliersCount << endl;

                    /* Calculate ratio of inliers */
                    float currRatio = inliersCount / (float)tmp_good_matches.size(); //  / ((float)keypoints_object_vector[i].size() + (float)keypoints_scene.size());
                    cout << "Ratio of Inlier: " << currRatio << endl;
                    cout << endl;
                    if (i == 0 || currRatio > ratio_max) {
                        ratio_max = currRatio;
                        inlier_max = inliersCount;
                        state_index = candidate_ids_object_vector[i];
                        H = tempH;
                    }
                    if (candidate_ids_object_vector[i] == curr_state_id) {
                        curr_state_ratio = currRatio;
                    }
                    if (candidate_ids_object_vector[i] == expected_state_id) {
                        expected_state_ratio = currRatio;
                    }
                }

                cout << "---- Max ratio within neighbors: " << ratio_max << endl;

                // Check if candidates include a good option
                // If not, search the original pool of reference images
                
                // Smoothing to get rid of noise 
                // For example, if curr state and new state have ratio 0.42 and 0.43,
                // stick with current state because this might be a noise.
                /*
                if (ratio_max > 0 && (ratio_max - curr_state_ratio) / ratio_max < small_diff_ratio && curr_state_ratio > benchmark_ratio * match_expectation_ratio) {
                    state_index = curr_state_id;
                    ratio_max = curr_state_ratio;
                }
                */

                // Compare with expected next state given fingertip location
                bool correct_next_state = false;
                /*
                bool correct_next_state = true;
                if (state_index != curr_state_id) {
                    if (curr_state_id < 0) {
                        correct_next_state = false;
                    } else {
                        correct_next_state = (expected_state_id == state_index);
                    }
                }
                */
                // Note: correct_next_state should be true when the state remains unchanged
                // Note: This should not be the case in build mode - not that much stability required
                /*
                if (!correct_next_state) {
                    if (ratio_max > 0 && (ratio_max - expected_state_ratio) / ratio_max < small_diff_ratio
                            && expected_state_ratio > benchmark_ratio * match_expectation_ratio) {
                        state_index = expected_state_id;
                        ratio_max = expected_state_ratio;
                    }
                }
                */

                // If current highest matching equals current state, then assume it's not changed;
                // Otherwise if lower than a threshold, match using the original pool of reference image
                if (!correct_next_state && ratio_max < benchmark_ratio * neighbor_ratio_threshold) {
                    cout << "Did not find a good match in neighbors of current state" << endl;
                    cout << "Searching all reference images...." << endl;
                    for (int i = 0; i < descriptors_object_gpu_vector.size(); i++) {
                        
                        // matching descriptors
                        BFMatcher matcher(NORM_L2);
                        Mat trainIdx, distance;
                        
                        /* CPU code */
                        vector<DMatch> matches;
                        matcher.match(descriptors_object_gpu_vector[i], descriptors_scene, matches);
                        // matcher.match(descriptors_object_gpu_vector[i], descriptors_scene, matches);

                        vector< DMatch > tmp_good_matches;
                        gpuFindGoodMatches(matches, tmp_good_matches);

                        // Can't find any object that matches the current scene
                        if (tmp_good_matches.size() < 4) {
                            cout << "-- Skipping Image -- " << i << endl;
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

                        cout << "-- Match with Image -- " << i << endl;
                        // cout << "OCR text: " << ocr_object_vector[i] << endl;
                        cout << "Object keypoints: " << keypoints_object_vector[i].size() << endl;
                        cout << "Scene keypoints: " << keypoints_scene.size() << endl;
                        cout << "Good matches: " << tmp_good_matches.size() << endl;
                        cout << "Inlier count: " << inliersCount << endl;

                        /* Calculate ratio of inliers */
                        float currRatio = inliersCount / (float)tmp_good_matches.size(); //  / ((float)keypoints_object_vector[i].size() + (float)keypoints_scene.size());
                        cout << "Ratio of Inlier: " << currRatio << endl;
                        cout << endl;
                        if (i == 0 || currRatio > ratio_max) {
                            ratio_max = currRatio;
                            inlier_max = inliersCount;
                            state_index = all_ids[i];
                            H = tempH;
                        }
                    }

                }
                // else {
                //     // Found in neighboring states
                //     confirmed_curr_state = true;
                // }
            }

            /******************** FindRect ********************/
            // Identify if it's a good match; take this away for now cuz unsure if it's stable
            /*
            img_object = img_object_vector[state_index];
            // Get the corners from the object (reference) image
            getImageCorners(img_object, object_corners);
            // Transform object corners to scene corners using H
            perspectiveTransform( object_corners, scene_corners, H);
            Rect rect;
            bool is_new_state_from_rect = !findRect(scene_corners, rect);
            */

            /******************** OCR ********************/
            float ratio_lower = nonmatched_ratio_threshold * benchmark_ratio;
            float ratio_upper = matched_ratio_threshold * benchmark_ratio;
            // Above matched_ratio_threshold * benchmark_ratio considered a match: this is consistent for all stages

            bool is_new_state;
            if (enableOCR) {
                is_new_state = isNewState(ratio_max, ratio_lower, ratio_upper, state_index, img_scene_raw, ocr_object_vector, ocr_threshold_upper, ocr_threshold_lower); // Using img_scene_raw instead of img_scene here
            } else {
                is_new_state = (ratio_max < ratio_upper);
            }

            // bool is_new_state = !confirmed_curr_state && isNewState(ratio_max, ratio_lower, ratio_upper, state_index, img_scene, ocr_object_vector, ocr_threshold_upper, ocr_threshold_lower);

            if (is_new_state) {      

            /******************** ADD NEW REF IMAGE ********************/

                // if (!enableAddNewRefImage) continue;
                // imshow("img_scene_raw", img_scene_raw);

                cout << "-- FOUND UNMATCHED IMAGE -- \n";
                new_img_pool.push_back(img_scene_raw);
                new_keypoints_pool.push_back(keypoints_scene);
                new_descriptors_pool.push_back(descriptors_scene);

                bool matched = checkNewImgFromPool(new_img_pool, new_keypoints_pool, new_descriptors_pool,
                    img_scene, keypoints_scene, descriptors_scene);

                if (!matched) {
                    cout << "No matched new image from pool!" << endl;
                    continue;
                }

                if (matched) { // when we find a "stable" new state in the poll

                    // vector<vector<int> > button_map;
                    // vector<string> true_labels;
                    // vector<vector<double> > button_center;
                    // double avg_button_size;
                    // labels_hash(filenames[i].substr(0, filenames[i].size()-9), img_object, ratio, button_map, true_labels, button_center, avg_button_size);
                    
                    // button_map_vector.push_back(button_map);
                    // true_labels_vector.push_back(true_labels);
                    // button_center_vector.push_back(button_center);
                    // avg_button_size_vector.push_back(avg_button_size);
                    
                    // cout << dir+filenames[i];
                    // lightingDetection(img_object);
                    
                    // Computer SURF keypoints and descriptors for object (reference) image
                    // Mat img_object_gpu;
                    // Mat img_object_gray;
                    // cvtColor(img_object, img_object_gray, CV_RGB2GRAY);
                    // img_object_gpu = img_object;
                    
                    // Mat keypoints_object_gpu;
                    // Mat descriptors_object_gpu;
                    // ORIGINAL
                    // surf(img_object_gpu, Mat(), keypoints_object_gpu, descriptors_object_gpu);
                    
                    // CPU ONLY
                    // vector<KeyPoint> keypoints_object;
                    // Mat descriptors_object; // SHOULD BE vector<float>
                    // int minHessian = 400;
                    // SurfFeatureDetector detector(minHessian);
                    // detector.detect(img_object_gpu, keypoints_object);
                    // printf("-- Keypoints Vector Size : %lu \n", keypoints_object.size() );
                    // //-- Step 2: Calculate reference descriptors (feature vectors)
                    // SurfDescriptorExtractor extractor;
                    // extractor.compute(img_object_gpu, keypoints_object, descriptors_object);
                    
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

                    cout << "UNMATCHED IMAGE MATCHED IN POOL \n";
                    // Mat img_object = img_scene;
                    img_object = img_scene_raw;
                    // img_scene.copyTo(img_object);
                    // img_object_vector.push_back(img_object);

                    cout << "-- ADDING NEW REFERENCE IMAGE -- \n";
                    state_index = all_ids.size();
                    cout << "state_index = " << state_index << endl; //  + ", Ratio = " + to_string(inlier_max / (float)keypoints_scene.size()) + "\n";

                    string new_name = state_index < 10 ? "New0" + to_string(state_index) + ".jpg" : "New" + to_string(state_index) + ".jpg"; 
                    string new_file = dir + new_name;

                    // sendTransitionInstructions(session_name, "Adding new state, please wait.");

                    if (read_mode == "local") {
                        files.push_back(new_name);
                        imwrite(new_file, img_object);
                        imwrite("../WebServer/images_original/" + session_name + "/" + new_name, img_object);
                    } else if (read_mode == "db") {  
                        
                        files.push_back(new_name);
                        imwrite(new_file, img_object);
                        imwrite("../WebServer/images_original/" + session_name + "/" + new_name, img_object);
                    
                        // if (!isStateProcessing(state_index)) {
                        cout << "HTTP request" << endl;

                        // DisonnectDB(DB);
                        // struct stat file_info;
                        // FILE *fd;
                        // fd = fopen("/Users/judy/Desktop/VizLens/VizLensDynamic/WebServer/images_original/New00.jpg", "r"); 
                        // fstat(fileno(fd), &file_info);

                        // if(!fd)
                        //     return 1; /* can't continue */ 
                        // /* to get the file size */ 
                        // if(fstat(fileno(fd), &file_info) != 0)
                        //     return 1; /* can't continue */ 

                        // imwrite(new_file, img_object);
                        // /*
                        CURL *hnd = curl_easy_init();
                        curl_easy_setopt(hnd, CURLOPT_CUSTOMREQUEST, "POST");
                        string url = "http://localhost:8888/VizLensDynamic/php/uploadOriginalforCVServer.php?userid=testuser&session=" + session_name + "&stateid=" + to_string(state_index);
                        curl_easy_setopt(hnd, CURLOPT_URL, url.c_str());
                        struct curl_slist *headers = NULL;
                        // headers = curl_slist_append(headers, "postman-token: 92191e21-8ce2-ff44-9c98-8cfd401b902f");
                        headers = curl_slist_append(headers, "cache-control: no-cache");
                        headers = curl_slist_append(headers, "content-type: multipart/form-data; boundary=----WebKitFormBoundary7MA4YWxkTrZu0gW");
                        curl_easy_setopt(hnd, CURLOPT_HTTPHEADER, headers);
                        string upload_file = "------WebKitFormBoundary7MA4YWxkTrZu0gW\r\nContent-Disposition: form-data; name=\"pictures[0]\"; filename=\"" + new_name + "\"\r\nContent-Type: image/jpeg\r\n\r\n\r\n------WebKitFormBoundary7MA4YWxkTrZu0gW--";
                        curl_easy_setopt(hnd, CURLOPT_POSTFIELDS, upload_file.c_str());
                        // curl_easy_setopt(hnd, CURLOPT_CONNECTTIMEOUT, 5L);
                        // curl_easy_setopt(hnd, CURLOPT_TIMEOUT, 5L);

                        // curl_easy_setopt(hnd, CURLOPT_READDATA, fd);
                        // curl_easy_setopt(hnd, CURLOPT_INFILESIZE_LARGE,(curl_off_t)file_info.st_size);
                        curl_easy_setopt(hnd, CURLOPT_VERBOSE, 1L);

                        CURLcode res = curl_easy_perform(hnd);
                        if (res != CURLE_OK) {
                            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
                        }
                        curl_easy_cleanup(hnd);
                    }

                    string input_line;
                    getline(cin, input_line); // Wait to prevent db tranffic jam

                    // isOpenDB = ConnectDB(DB);
                    // if (isOpenDB) cout << "Connected Successful" << endl;
                    // else cout << "connection failed " << endl;

                    // TODO: Need to store OCR to data base
                    // Update img_object to cropped image read from database
                    updateCroppedImage(session_name, dir_server, new_name, state_index, img_object);
                    img_object_vector.push_back(img_object);
                    // enableAddNewRefImage = false;

                    // TODO: Might need to change text_scene to updated OCR? Or not important?
                    // ocr_object_vector.push_back(text_scene);
                    cout << "Added new state to transition graph" << endl;
                    addStateToTransitionGraph(session_name, transition_graph, state_index);

                    // Add OCR of scene to vector;
                    String text_object;
                    if (enableOCR) {
                        getOCRTextFromImage(img_object, text_object);
                    }

                    files.push_back(new_name);
                    all_ids.push_back(state_index);
                    objectnames.push_back(new_name);
                    if (enableOCR) {
                        ocr_object_vector.push_back(text_object);
                    }

                    // Can add this line to store image locally
                    
                    // Re-detect keypoints & descriptors
                    // using keypoints and descriptors of cropped object
                    vector<KeyPoint> keypoints_object;
                    Mat descriptors_object; // SHOULD BE vector<float>
                    detector.detect(img_object, keypoints_object);
                    SurfDescriptorExtractor extractor;
                    extractor.compute(img_object, keypoints_object, descriptors_object);
        
                    keypoints_object_vector.push_back(keypoints_scene);
                    descriptors_object_gpu_vector.push_back(descriptors_scene);

                    // VisDynamic: May need crowdsourcing for labeling buttons in added image object
                    vector<int> button_id;
                    vector<vector<int> > button_map;
                    vector<string> true_labels;
                    vector<vector<double> > button_center;
                    double avg_button_size;
                    double obj_ratio = img_object.rows > img_object.cols ? 600.0 / (double)img_object.rows : 600.0 / (double) img_object.cols;
                    Size object_size(img_object.cols * obj_ratio, img_object.rows * obj_ratio); //(150, 445)
                    resize(img_object, img_object, object_size);
                    labels_hash(session_name, state_index, img_object, obj_ratio, button_id, button_map, true_labels, button_center, avg_button_size);
                
                    button_map_vector.push_back(button_map);
                    true_labels_vector.push_back(true_labels);
                    button_center_vector.push_back(button_center);
                    avg_button_size_vector.push_back(avg_button_size);
                    // cout << "BEFORE PUSHING \n";
                    // button_map_vector.push_back(button_map_vector[button_map_vector.size() - 1]);
                    // true_labels_vector.push_back(true_labels_vector[true_labels_vector.size() - 1]);
                    // button_center_vector.push_back(button_center_vector[button_center_vector.size() - 1]);
                    // avg_button_size_vector.push_back(avg_button_size_vector[avg_button_size_vector.size() - 1]);
                    // cout << "AFTER PUSHING \n";
                    // string tmp_string;
                    // getImageToProcess(tmp_string);
                    description_text_vector.push_back(getStateDescription(session_name, state_index));

                    // Get keypoints and H for the newly added current state
                    BFMatcher matcher(NORM_L2);
                    Mat trainIdx, distance;
                    /* CPU code */
                    vector<DMatch> matches;
                    matcher.match(descriptors_object, descriptors_scene, matches);
                    vector< DMatch > tmp_good_matches;
                    gpuFindGoodMatches(matches, tmp_good_matches);
                    vector<uchar> inliers;
                    findH(keypoints_object, keypoints_scene, tmp_good_matches, H, inliers);

                    sendTransitionInstructions(session_name, "New state ready, please resume.");
                }
            } else {
                new_img_pool.clear();
                new_keypoints_pool.clear();
                new_descriptors_pool.clear();
                cout << "Not considered a new state!" << endl;
            }
                
            // Use the history of detected screens to determine the current screen and get rid of noises.
            // Point2f transition_fingertip_location;
            if (detected_states.size() == state_transition_smoothing) {
                detected_states.erase(detected_states.begin());
                // detected_fingertip_locations.erase(detected_fingertip_locations.begin());
            }
            detected_states.push_back(state_index);
            // detected_fingertip_locations.push_back(fingertip_location[0]);
            string matched_state;
            string current_state;
            int new_state_id = findCurrentState(detected_states);
            if (new_state_id != curr_state_id) {
                prev_state_id = curr_state_id;
                curr_state_id = new_state_id;
                curr_state_img_scene = img_scene;
                benchmark_ratio = max(benchmark_ratio, ratio_max);
                // transition_fingertip_location = findTransitionFingertipLocation(new_state_id, detected_states, detected_fingertip_locations);
                updateTransitionGraph(session_name, transition_graph, prev_state_id, curr_state_id, -1);
            }
            cout << "Current state id: "<< curr_state_id <<endl;

            enableAddNewRefImage = true;
            // TODO(judy): For now use button_id = -1 when no finger is detected
            // Should change this to current finger location
            // TODO(judy): Move this to above
            
            cout << "PREVIOUS STATE: " << prev_state_id << endl;
            cout << "CURRENT STATE: " << curr_state_id << endl;

            /******************** Fingertip Location ********************/
            // As well as preparation for expected state in the next step
            img_object = img_object_vector[curr_state_id];
            button_map = button_map_vector[curr_state_id];
            true_labels = true_labels_vector[curr_state_id];
            button_center = button_center_vector[curr_state_id];
            avg_button_size = avg_button_size_vector[curr_state_id];

            // Warp scene image with finger to reference image size
            Mat img_skin;
            warpPerspective(img_scene, img_skin, H.inv(), img_object.size(), INTER_LINEAR, BORDER_CONSTANT);
            // Filter skin color to get binary image
            Mat img_skin_binary;
            // skinColorAdjustment(img_object, img_skin);
            skinColorFilter(img_skin, img_skin_binary);
            // skinColorSubtractor(img_skin, img_object, img_skin_binary);    
              
            // Note: Get this out for now  
            /*
            // Find fingertip location
            fingertip_location[0].x = -1;
            fingertip_location[0].y = -1;
            findFingertipLocation(img_skin_binary, img_object.rows * img_object.cols, H, avg_button_size, fingertip_location);
            // Find relative fingertip location in scene image
            perspectiveTransform( fingertip_location, scene_fingertip_location, H);
            
            // Get index of button the finger is currently above    
            button_index = getButtonIndex(fingertip_location[0], button_map, true_labels);

            // Get expected next state for next round of matching
            expected_state_id = getExpectedNextState(transition_graph, curr_state_id, button_index); 
            */

            cout << "Current button index: " << button_index << endl;
            cout << "Expected state id: " << expected_state_id << endl;

            /******************** Send Feedback ********************/

            // double object_ratio = img_scene.rows > img_object.cols ? 200.0 / (double)img_object.rows : 200.0 / (double) img_object.cols;
            // Size img_object_size(img_object.cols * object_ratio, img_object.rows * object_ratio); //(150, 445)
            // resize(img_object, img_object, img_object_size);

            /* Old visualization 
            drawMatches( img_object, keypoints_object_vector[state_index], img_scene, keypoints_scene_gpu,
                        good_matches, img_results, Scalar::all(-1), Scalar::all(-1),
                        vector<char>(), DrawMatchesFlags::NOT_DRAW_SINGLE_POINTS );
            */

            // Resize to fit expected size
            double scene_ratio = img_scene.rows > (double)img_scene.cols / 3 * 2 ? 500 / (double)img_scene.rows : 750 / (double)img_scene.cols;
            Size img_scene_size(img_scene.cols * scene_ratio, img_scene.rows * scene_ratio); //(150, 445)
            resize(img_scene, img_scene, img_scene_size);

            // DRAW MAT
            Mat img_results = Mat::zeros( 750, 1200, CV_8UC3 );
            drawScreenMap(img_results, img_scene, img_object_vector, curr_state_id, all_ids);
            drawStateGraph(img_results, transition_graph, curr_state_id);

            
            imshow( "Overall Results", img_results );

            toc = clock();
            printf("Elapsed: %f seconds\n", (double)(toc - tic) / CLOCKS_PER_SEC);

            // ****** END CLOCK ******
            // updateImage(db_object_found, db_finger_found, fingerx, fingery);
            idle = 1;
            
            waitKey(1); // waits to display frame
        } catch (Exception &e) {
            idle = 1;
            skipImage(); // To be added when sqlite is back
            // continue;
        }
    }

    // curl_easy_cleanup(hnd);

    waitKey(0);
    return 0;
}
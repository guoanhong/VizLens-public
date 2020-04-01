#include <iostream>
#include <fstream>
#include <unistd.h>
#include "curl/curl.h"
#include "opencv2/nonfree/nonfree.hpp"
#include <opencv2/gpu/gpu.hpp>
#include "opencv2/nonfree/gpu.hpp"
#include "sqlite3.h"

#include "lib/access/db_utils.hpp"
#include "lib/access/img_utils.hpp"
// #include "lib/ocr_utils.hpp"
#include "lib/access/statemap_utils.hpp"
#include "lib/access/feedback_utils.hpp"
#include "lib/access/visualization.hpp"

#define DB "/home/guoanhong/Desktop/VizLensDynamic/WebServer/db/access_14/VizLensDynamic-14.db"
#define imageDirBase "/home/guoanhong/Desktop/VizLensDynamic/WebServer/images_video/"
#define dirBase "/home/guoanhong/Desktop/VizLensDynamic/CVServer/samples/"

const string session_name = "coffee_machine";
const string read_mode = "db";

using namespace cv;
using namespace std;
using namespace cv::gpu;
// using namespace tesseract;

const int MIN_FINGER_DEPTH = 20;
const int MAX_FINGER_ANGLE = 60;
const double PI = 3.1415926;

bool isOpenDB = false;
sqlite3 *dbfile;

string id = "0";
string image = "";
string mode = "3"; // Guide by path mode 
string target = "";
string state = "";
string feedback = "";
string guidance = "";
int idle = 1;
int curr_state_id = -1;
Mat curr_state_img_scene;
float benchmark_ratio = 1.0;

int num_max_object = 14;
int num_max_frame[14] = {1, 3, 5, 7, 11, 13, 15, 17, 18, 20, 22, 24, 28, 30};

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

string uniqueFilename(int frame) {
    if (frame < 10) {
        return "00" + to_string(frame);
    } else if (frame < 100) {
        return "0" + to_string(frame);
    }
    return to_string(frame);
}

/** @function main */
int main(int argc, char** argv)
{
    cout << "Start (dynamic display for GPU)"<< endl;

    clock_t initializeStartClock = clock();

    string folder = "hcii_coffee"; // string(argv[1]);
    // string video = string(argv[2]); // Example: "video_coffee_hard.MOV";

    string log_dir = string(imageDirBase) + session_name + "/log/";
    ofstream file_trace(log_dir + "log_trace" + to_string((double)initializeStartClock) + ".txt");
    ofstream file_timer(log_dir + "log_timer" + to_string((double)initializeStartClock) + ".txt");

    cv::gpu::printShortCudaDeviceInfo(cv::gpu::getDevice());

    // METADATA: Thresholds
    float small_diff_ratio = 0.05; // If diff of current state and detected state is below this value, consider it a noise
    float match_expectation_ratio = 0.6; // If ratio of expected state is above this value, consider it a match
    float neighbor_ratio_threshold = 0.3; // If max ratio among neighbors is above this value, consider it a match
    // This value is set lower than that in build mode to allow more errors

    float matched_ratio_threshold = 0.0; // atof(argv[2]); // 0.3; // Above this value, consider it as a match

    /* Initialize vectors to store reference images and buttons */
    vector<Mat> img_object_vector = vector<Mat>();
    vector<vector<int>> button_id_vector = vector<vector<int>>();
    vector<vector<vector<int> > > button_map_vector = vector<vector<vector<int> > >();
    vector<vector<string> > true_labels_vector = vector<vector<string> >();
    vector<vector<vector<double> > > button_center_vector = vector<vector<vector<double> > >(); 
    vector<vector<vector<double> > > button_shape_vector = vector<vector<vector<double> > >(); 
    vector<double> avg_button_size_vector = vector<double>();

    vector<GpuMat> img_object_gpu_vector = vector<GpuMat>();
    vector<GpuMat> keypoints_object_gpu_vector = vector<GpuMat>();
    vector<GpuMat> descriptors_object_gpu_vector = vector<GpuMat>();

    vector<vector<KeyPoint> > keypoints_object_vector = vector<vector<KeyPoint> >();
    vector<vector<float> > descriptors_object_vector = vector<vector<float> >();
    // vector<string> ocr_object_vector = vector<string>();

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
    
    // TODO: might not be used
    vector<GpuMat> new_img_pool_gpu;
    vector<GpuMat> new_keypoints_pool_gpu = vector<GpuMat>();
    vector<GpuMat> new_descriptors_pool_gpu = vector<GpuMat>();

    vector<int> state_path = vector<int>();
    vector<int> button_path = vector<int>();

    /******************** DB Connection ********************/
    isOpenDB = ConnectDB(DB);
   
    if (isOpenDB) cout << "Connected Successful" << endl;
    else cout << "connection failed " << endl;

    /******************** Read Expected Path ******************/
    getExpectedPath(state_path, button_path);
    if (state_path.size() == 0) {
        cout << "State path is empty!" << endl;
        return 1;
    } else if (state_path.size() != button_path.size() + 1) {
        cout << "State path and button path not of expected size!" << endl;
        return 1;
    }
    int next_idx_on_path = 0;
    bool success = false;

    /******************** Read Reference Images (objects) ********************/     
    string dir = string(dirBase) + folder + "/images/";
    string dir_server = "/home/guoanhong/Desktop/VizLensDynamic/WebServer/images_reference/" + session_name + "/";
    getRefImageFilenames(session_name, dir, filenames, read_mode);
    getTransitionGraph(session_name, read_mode, filenames, transition_graph);

    // TODO(judy): Need to read OCR from db; add an OCR column in objects table
    /*
    ifstream inFileOCR;
    inFileOCR.open(dir + "../log/ocr.txt");
    string line;
    while (inFileOCR >> curr_ocr) {
        ocr_object_vector.push_back(curr_ocr);
    }
    while (getline(inFileOCR, line))
    {
        ocr_object_vector.push_back(line);
    }
    cout << ocr_object_vector.size() << endl;
    inFileOCR.close();
    */
    
    /* Load object images from WebServer image_reference folder */
    string ending = "jpg";

    /* GPU code */
    SURF_GPU surf;
    surf.hessianThreshold = 400;
    
    // int minHessian = 400;
    // SurfFeatureDetector detector(minHessian);
    
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
            vector<vector<double> > button_shape;
            double avg_button_size;
            labels_hash(session_name, id_count - 1, img_object, ratio, button_id, button_map, true_labels, button_center, button_shape, avg_button_size);
            // labels_hash(filenames[i].substr(0, filenames[i].size()- 4), img_object, ratio, button_map, true_labels, button_center, avg_button_size);

            button_id_vector.push_back(button_id);
            button_map_vector.push_back(button_map);
            true_labels_vector.push_back(true_labels);
            button_center_vector.push_back(button_center);
            button_shape_vector.push_back(button_shape);
            avg_button_size_vector.push_back(avg_button_size);
            
            cout << dir+filenames[i] << endl;
            lightingDetection(img_object);
            
            /* Computer SURF keypoints and descriptors for object (reference) image */
            GpuMat img_object_gpu;
            Mat img_object_gray;
            cvtColor(img_object, img_object_gray, CV_RGB2GRAY);
            img_object_gpu.upload(img_object_gray);

            GpuMat keypoints_object_gpu;
            GpuMat descriptors_object_gpu;
            /* GPU code */
            surf(img_object_gpu, GpuMat(), keypoints_object_gpu, descriptors_object_gpu);
            
            /* CPU code */
            // vector<KeyPoint> keypoints_object;
            // Mat descriptors_object; // GPU: vector<float>
            // int minHessian = 400;
            // SurfFeatureDetector detector(minHessian);
            // detector.detect(img_object_gpu, keypoints_object);
            // printf("-- Keypoints Vector Size : %lu \n", keypoints_object.size() );
            // /* Calculate reference descriptors (feature vectors) */
            // SurfDescriptorExtractor extractor;
            // extractor.compute(img_object_gpu, keypoints_object, descriptors_object);
            
            /* GPU code */
            img_object_gpu_vector.push_back(img_object_gpu);
            printf("-- Keypoints Vector Size : %lu \n", keypoints_object_gpu.cols );
            
            // Download objects results
            vector<KeyPoint> tmp_keypoints_object;
            vector<float> tmp_descriptors_object;            
            surf.downloadKeypoints(keypoints_object_gpu, tmp_keypoints_object);
            surf.downloadDescriptors(descriptors_object_gpu, tmp_descriptors_object);
            
            all_ids.push_back(id_count - 1);
            objectnames.push_back(filenames[i].substr(0, filenames[i].size() - 4));
            keypoints_object_vector.push_back(tmp_keypoints_object);
            descriptors_object_vector.push_back(tmp_descriptors_object);
            keypoints_object_gpu_vector.push_back(keypoints_object_gpu);
            descriptors_object_gpu_vector.push_back(descriptors_object_gpu);
        }
    }

    // TODO(judy): Need to DEBUG this part
    if (hasReadyToReadObjects()) {
        int new_state;
        string new_name;
        Mat new_object;
        updateCroppedImage(session_name, dir_server, new_name, new_state, new_object);
                            
        // Computer SURF keypoints and descriptors for object (reference) image
        GpuMat new_object_gpu;
        Mat new_object_gray;
        cvtColor(new_object_gpu, new_object_gray, CV_RGB2GRAY);
        new_object_gpu.upload(new_object_gray);
          
        GpuMat new_keypoints_object_gpu;
        GpuMat new_descriptors_object_gpu;
        // GPU code
        surf(new_object_gpu, GpuMat(), new_keypoints_object_gpu, new_descriptors_object_gpu);
        
        // CPU code
        // vector<KeyPoint> new_keypoints_object;
        // Mat new_descriptors_object; // GPU: vector<float>
        // int minHessian = 400;
        // SurfFeatureDetector detector(minHessian);
        // detector.detect(new_object_gpu, new_keypoints_object);
        // printf("-- Keypoints Vector Size : %lu \n", new_keypoints_object.size() );
                
        // // Calculate reference descriptors (feature vectors)
        // SurfDescriptorExtractor extractor;
        // extractor.compute(new_object_gpu, new_keypoints_object, new_descriptors_object);
         
        img_object_vector.push_back(new_object);
        img_object_gpu_vector.push_back(new_object_gpu);
        printf("-- Keypoints Vector Size new object : %lu \n", new_keypoints_object_gpu.cols );

        // downloading objects results
        vector<KeyPoint> new_keypoints_object;
        vector<float> new_descriptors_object;
        
        // ORIGINAL
        surf.downloadKeypoints(new_keypoints_object_gpu, new_keypoints_object);
        surf.downloadDescriptors(new_descriptors_object_gpu, new_descriptors_object);

        all_ids.push_back(new_state);
        objectnames.push_back(new_name);
        keypoints_object_vector.push_back(new_keypoints_object);
        descriptors_object_vector.push_back(new_descriptors_object);
        keypoints_object_gpu_vector.push_back(new_keypoints_object_gpu);
        descriptors_object_gpu_vector.push_back(new_descriptors_object_gpu);

        // VizDynamic: May need crowdsourcing for labeling buttons in added image object
        vector<int> button_id;
        vector<vector<int> > button_map;
        vector<string> true_labels;
        vector<vector<double> > button_center;
        vector<vector<double> > button_shape;
        double avg_button_size;
        double obj_ratio = new_object.rows > new_object.cols ? 600.0 / (double)new_object.rows : 600.0 / (double) new_object.cols;
        Size object_size(new_object.cols * obj_ratio, new_object.rows * obj_ratio); //(150, 445)
        resize(new_object, new_object, object_size);
        labels_hash(session_name, new_state, new_object, obj_ratio, button_id, button_map, true_labels, button_center, button_shape, avg_button_size);
            
        button_map_vector.push_back(button_map);
        true_labels_vector.push_back(true_labels);
        button_center_vector.push_back(button_center);
        button_shape_vector.push_back(button_shape);
        avg_button_size_vector.push_back(avg_button_size);
    }

    for (int i = 0; i < all_ids.size(); i++) {
        description_text_vector.push_back(getStateDescription(session_name, i));
        cout << "Description Text: " << description_text_vector[i] << endl;
    }

    clock_t initializeEndClock = clock();
    double initializeDurationClock = (double)(initializeEndClock - initializeStartClock) / CLOCKS_PER_SEC;
    if (file_timer.is_open()) {
        file_timer << initializeDurationClock << endl;
    }
    printf("Initialize: %f seconds\n", initializeDurationClock);

    /******************** Read Video (scenes) and Match with Reference Images (objects) ********************/
    
    /* Initialize values of detected screen and fintertip location*/
    GpuMat keypoints_scene_gpu;
    GpuMat descriptors_scene_gpu;
    
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
    
    bool force_state_change = false;

    /* CPU code */
    // Video mode
    string video_name = string(dirBase) + "access_mode/hcii_coffee.mov"; // folder + "/videos/" + video;
    VideoCapture capture(video_name);
    if (!capture.isOpened()) {
        throw "--(!) Error reading steam";
    }
    int frame = 0;
    // int first_skip_frames = 0;
    // for (int i = 0; i < first_skip_frames; i++) { capture.grab(); }
    // frame += first_skip_frames;
    
    Mat H;
    int i;
    id = "0";

    /* Start capturing scenes and matching with reference images */
    for ( ; !success && frame <= num_max_frame[num_max_object - 1] * 30; ) {
        try {
            /* Read scene */

            // Scene doesn't read in valid image
            // Update cropped image doesn't change it to read

            // Read from video streaming
            /*
            string image_to_process;
            while (idle == 1) {
                if(clock() - last_time > (double)(0.05 * 1000000)) {
                    count++;
                    getImageToProcess(image_to_process);
                    last_time = clock();
                }
            }
            */

            int frames_skipped = 3;
            // Mat img_scene = imread(imageDirBase + session_name + "/" + image_to_process);
            // cout << imageDirBase + session_name + "/" + image_to_process << endl;
            // cout << "reading img_scene" << endl;

            /****** START CLOCK *******/
            clock_t loopStartClock = clock();

            Mat img_scene;
            for (int a = 0;  a < frames_skipped - 1; a++) {
                frame++;
                capture.grab();
            }
            frame++;
            capture >> img_scene;
            if (img_scene.empty()) break;
            /* Use local videos for image stream */

            // Mat img_scene = imread(imageDirBase + session_name + "/" + image_to_process);
            // cout << imageDirBase + session_name + "/" + image_to_process << endl;
            // cout << "reading img_scene" << endl;
            
            // double img_scene_ratio = img_scene.rows > img_scene.cols ? 1280.0 / (double)img_scene.rows : 1280.0 / (double) img_scene.cols;

            // Size img_scene_size(img_scene.cols * img_scene_ratio, img_scene.rows * img_scene_ratio);
            // resize(img_scene, img_scene, img_scene_size);

            cout << "img_scene size " << img_scene.cols << img_scene.rows << endl;

            // ++i;

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
            
            /* Computer SURF keypoints and descriptors for scene (video) image */
            GpuMat img_scene_gpu;
            Mat img_scene_gray;
            cvtColor(img_scene, img_scene_gray, CV_RGB2GRAY);
            img_scene_gpu.upload(img_scene_gray);
            surf(img_scene_gpu, GpuMat(), keypoints_scene_gpu, descriptors_scene_gpu);
            
            // img_scene_gpu = img_scene_gray;
            // /* CPU code */
            // vector<KeyPoint> keypoints_scene;
            // Mat descriptors_scene; // SHOULD BE vector<float>
            // detector.detect(img_scene_gpu, keypoints_scene);
            // SurfDescriptorExtractor extractor;
            // extractor.compute(img_scene_gpu, keypoints_scene, descriptors_scene);
            
            cout << "FOUND " << keypoints_scene_gpu.cols << " keypoints on second image" << endl;
            
            if (keypoints_scene_gpu.cols == 0) {
                idle = 1;
                skipImage();
                continue;
            }
            
            /* GPU code: */
            vector<KeyPoint> keypoints_scene;
            vector<float> descriptors_scene;
            surf.downloadKeypoints(keypoints_scene_gpu, keypoints_scene);
            surf.downloadDescriptors(descriptors_scene_gpu, descriptors_scene);

            clock_t loopSURFClock = clock();
            double loopStartSURFDurationClock = (double)(loopSURFClock - loopStartClock) / CLOCKS_PER_SEC;
            if (file_timer.is_open()) {
                file_timer << loopStartSURFDurationClock << ",";
            }
            printf("Loop start-SURF: %f seconds\n", loopStartSURFDurationClock);

            /******************** Start Matching ********************/

            // FINGERTIP LOCATION
            Mat img_object;
            vector<vector<int> > button_map;
            vector<vector<double> > button_center;
            vector<vector<double> > button_shape;
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

            int state_index = 0;
            float ratio_max = 0;

            // Reduce search space: See if the scene stays on the same image
            // Reduce search space: See if the scene is the expected image
            if (curr_state_id >= 0 && matchesCurrentStateGPU(descriptors_scene_gpu, keypoints_scene, curr_state_img_scene, descriptors_object_gpu_vector[curr_state_id], keypoints_object_vector[curr_state_id], benchmark_ratio, match_expectation_ratio)) {
                confirmed_curr_state = true;
                state_index = curr_state_id;
                if (file_trace.is_open()) {
                    file_trace << "current" << ",";
                }
                cout << "---- Successfully matched with CURRENT state" << endl;
            } else if (expected_state_id >= 0 && matchesExpectedStateGPU(descriptors_scene_gpu, keypoints_scene, descriptors_object_gpu_vector[expected_state_id], keypoints_object_vector[expected_state_id], benchmark_ratio, match_expectation_ratio)) {
                confirmed_curr_state = true;
                state_index = expected_state_id;
                if (file_trace.is_open()) {
                    file_trace << "expected" << ",";
                }
                cout << "---- Successfully matched with EXPECTED state" << endl;
            } else {

                int inlier_max = -1;
                
                vector< DMatch > good_matches;
                vector<int> candidate_ids_object_vector;
                vector<GpuMat> candidate_descriptors_object_gpu_vector;
                
                getCandidateObjectIds(curr_state_id, all_ids, transition_graph, candidate_ids_object_vector);
                getCandidateObjectDescriptorsGPU(candidate_ids_object_vector, descriptors_object_gpu_vector, candidate_descriptors_object_gpu_vector);
                    
                for (int i = 0; i < candidate_ids_object_vector.size(); i++) {
                    cout << "---- NEIGHBOR OF CURRENT STATE : " << candidate_ids_object_vector[i] << endl;
                }
                    
                cout << "------------------------------------------\n";
                cout << "Start Matching \n";
                cout << "------------------------------------------\n";

                float curr_state_ratio = 0.0;
                float expected_state_ratio = 0.0;
                for (int i = 0; i < candidate_descriptors_object_gpu_vector.size();i++) {

                    // matching descriptors
                    BFMatcher_GPU matcher(NORM_L2);
                    GpuMat trainIdx, distance;

                    matcher.matchSingle(candidate_descriptors_object_gpu_vector[i], descriptors_scene_gpu, trainIdx, distance);
                    vector<DMatch> matches;
                    BFMatcher_GPU::matchDownload(trainIdx, distance, matches);

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
                if (ratio_max > 0 && (ratio_max - curr_state_ratio) / ratio_max < small_diff_ratio && curr_state_ratio > benchmark_ratio * match_expectation_ratio) {
                    state_index = curr_state_id;
                    ratio_max = curr_state_ratio;
                }

                // Compare with expected next state given fingertip location
                bool correct_next_state = true;
                if (state_index != curr_state_id) {
                    if (curr_state_id < 0) {
                        correct_next_state = false;
                    } else {
                        correct_next_state = (expected_state_id == state_index);
                    }
                }
                // Note: correct_next_state should be true when the state remains unchanged
                if (!correct_next_state) {
                    if (ratio_max > 0 && (ratio_max - expected_state_ratio) / ratio_max < small_diff_ratio
                            && expected_state_ratio > benchmark_ratio * match_expectation_ratio) {
                        state_index = expected_state_id;
                        ratio_max = expected_state_ratio;
                    }
                }

                // If current highest matching equals current state, then assume it's not changed;
                // Otherwise if lower than a threshold, match using the original pool of reference image
                if (state_index != curr_state_id && !correct_next_state && ratio_max < benchmark_ratio * neighbor_ratio_threshold) {
                    cout << "Did not find a good match in neighbors of current state" << endl;
                    cout << "Searching all reference images...." << endl;
                    for (int i = 0; i < descriptors_object_gpu_vector.size(); i++) {
                            
                        // matching descriptors
                        BFMatcher_GPU matcher(NORM_L2);
                        GpuMat trainIdx, distance;
                        
                        matcher.matchSingle(descriptors_object_gpu_vector[i], descriptors_scene_gpu, trainIdx, distance);
                        vector<DMatch> matches;
                        BFMatcher_GPU::matchDownload(trainIdx, distance, matches);

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

                } else {
                    // Found in neighboring states
                    confirmed_curr_state = true;
                }
            }

            // Through away current scene if max ratio is below certain threshold
            Rect rect;
            bool no_object = !findRect(scene_corners, rect) && (ratio_max < matched_ratio_threshold);
                
            bool state_change = false;
            // Use the history of detected screens to determine the current screen and get rid of noises.
            // Point2f transition_fingertip_location;
            if (!no_object) {
                if (detected_states.size() == 6) {
                    detected_states.erase(detected_states.begin());
                    // detected_fingertip_locations.erase(detected_fingertip_locations.begin());
                }
                detected_states.push_back(state_index);
                // detected_fingertip_locations.push_back(fingertip_location[0]);
                string matched_state;
                string current_state;
                int new_state_id = findCurrentState(detected_states);
                state_change = (new_state_id != curr_state_id);
                if (state_change || force_state_change) {
                    prev_state_id = curr_state_id;
                    curr_state_id = new_state_id;
                    curr_state_img_scene = img_scene;
                    benchmark_ratio = max(benchmark_ratio, ratio_max);
                    force_state_change = false;
                    // transition_fingertip_location = findTransitionFingertipLocation(new_state_id, detected_states, detected_fingertip_locations);
                }
                cout << "Current state id: "<< curr_state_id <<endl;
            } else {
                cout << "No object matched for current scene!" << endl;
            }
            /**** End of matching for lockStateTransition = false ****/
            
            cout << "PREVIOUS STATE: " << prev_state_id << endl;
            cout << "CURRENT STATE: " << curr_state_id << endl;

            clock_t loopMatchingClock = clock();
            double loopSURFMatchingDurationClock = (double)(loopMatchingClock - loopSURFClock) / CLOCKS_PER_SEC;
            if (file_timer.is_open()) {
                file_timer << loopSURFMatchingDurationClock << ",";
            }
            printf("Loop SURF-matching: %f seconds\n", loopSURFMatchingDurationClock);

            /******************** Fingertip Location ********************/
            // As well as preparation for expected state in the next step
            fingertip_location[0].x = -1;
            fingertip_location[0].y = -1;
            if (!no_object) {
                img_object = img_object_vector[curr_state_id];
                button_map = button_map_vector[curr_state_id];
                button_center = button_center_vector[curr_state_id];
                button_shape = button_shape_vector[curr_state_id];
                true_labels = true_labels_vector[curr_state_id];
                avg_button_size = avg_button_size_vector[curr_state_id];
                
                // Get the corners from the object (reference) image
                getImageCorners(img_object, object_corners);
                // Transform object corners to scene corners using H
                perspectiveTransform( object_corners, scene_corners, H);

                // Warp scene image with finger to reference image size
                Mat img_skin;
                warpPerspective(img_scene, img_skin, H.inv(), img_object.size(), INTER_LINEAR, BORDER_CONSTANT);
                // Filter skin color to get binary image
                Mat img_skin_binary;
                // skinColorAdjustment(img_object, img_skin);
                skinColorFilter(img_skin, img_skin_binary);
                // skinColorSubtractor(img_skin, img_object, img_skin_binary);    
                    
                findFingertipLocation(img_skin_binary, img_object.rows * img_object.cols, H, avg_button_size, fingertip_location);
                // Find relative fingertip location in scene image
                perspectiveTransform( fingertip_location, scene_fingertip_location, H);
                    
                button_index = getButtonIndex(fingertip_location[0], button_map, true_labels);
                expected_state_id = getExpectedNextState(transition_graph, curr_state_id, button_index);
            }

            /******************** Send Feedback ********************/

            // double object_ratio = img_scene.rows > img_object.cols ? 200.0 / (double)img_object.rows : 200.0 / (double) img_object.cols;
            // Size img_object_size(img_object.cols * object_ratio, img_object.rows * object_ratio); //(150, 445)
            // resize(img_object, img_object, img_object_size);

            /* Old visualization 
            drawMatches( img_object, keypoints_object_vector[state_index], img_scene, keypoints_scene_gpu,
                        good_matches, img_results, Scalar::all(-1), Scalar::all(-1),
                        vector<char>(), DrawMatchesFlags::NOT_DRAW_SINGLE_POINTS );
            */

            // Disable for tech evaluation
            // double scene_ratio = 1.0;
            
            // Resize to fit expected size
            double scene_ratio = img_scene.rows > (double)img_scene.cols / 3 * 2 ? 500 / (double)img_scene.rows : 750 / (double)img_scene.cols;
            Size img_scene_size(img_scene.cols * scene_ratio, img_scene.rows * scene_ratio); //(150, 445)
            resize(img_scene, img_scene, img_scene_size);

            // DRAW MAT
            Mat img_results = Mat::zeros( 750, 1200, CV_8UC3 );
            drawScreenMap(img_results, img_scene, img_object_vector, curr_state_id, all_ids);
            drawStateGraph(img_results, transition_graph, curr_state_id);
            

            // DISPLAY LOG
            /*
            if (!lockStateTransition) {
                displayLogOnScreen(img_results, objectnames, detected_states, "", "", "");// ocr_object_vector[curr_state_id]);
            }
            */

            // // UPDATED FINGERTIP LOCATION
            // // Get the corners from the object (reference) image
            // getImageCorners(img_object, object_corners);
            // // Transform object corners to scene corners using H
            // perspectiveTransform( object_corners, scene_corners, H);


            int guidance_index = -2; // = -2;

            // Set mode to -2 for testing
            if (!no_object) {
                if (stoi(mode) == 2) {
                    guidance_index = getGuidanceIndex(true_labels, target);
                } else if (stoi(mode) == 3) {
                    // Using current screen id, find out what's the button to press to go to next state
                    guidance_index = getGuidanceIndexBasedOnExpectedPath(state_path, button_path, transition_graph, button_id_vector, curr_state_id, next_idx_on_path);
                    cout << "Guidance Mode: Based on Expected Path" << endl;
                    cout << "Current State Id: " << curr_state_id << endl;
                    cout << "Next Target State Id: " << state_path[next_idx_on_path] << endl;
                    cout << "Guidance Button: " << guidance_index << endl;
                    if (next_idx_on_path >= state_path.size()) {
                        success = true;
                        force_state_change = true;
                    }
                }
            }
            // Originally declared here
            // vector<Point2f> fingertip_location(1);
            bool announcing_state = false;
            if (success) {
                string currfeedback = description_text_vector[curr_state_id];
                feedback = "State: " + currfeedback + "; Successfully reached target state!";
                cout << "Successfully reached target state! Done." << endl;
            } else if (guidance_index == -1) {
                if (no_button % 5 == 1) {
                    feedback = "no button";
                } else {
                    feedback = "";
                }
                no_button++;
            } else {
                no_button = 1;

                Rect rect;
                if (no_object || !findRect(scene_corners, rect)) {
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
                        force_state_change = true;
                    } else {
                        feedback = "";
                        object_not_found++;
                    }

                    cout << "No Object" << endl ;
                } else {
                    db_object_found = 1;

                    object_not_found = 1;
                            
                    scene_fingertip_location[0].x *= scene_ratio;
                    scene_fingertip_location[0].y *= scene_ratio;

                    if (state_change || force_state_change) {
                        announcing_state = true;
                        string currfeedback = description_text_vector[curr_state_id];
                        if (stoi(mode) == 1) {
                            feedback = "State: " + currfeedback;
                        } else if (stoi(mode) == 2 || stoi(mode) == 3) {
                            feedback = "State: " + currfeedback;
                            if (guidance_index >= 0) {
                                feedback += "; Target: " + true_labels[guidance_index];
                            }
                        }
                        force_state_change = false;
                    } else if (fingertip_location[0].x == -1 && fingertip_location[0].y == -1) {

                        // Point2f object_center_scene;
                        // object_center_scene = scene_corners[4];

                        // feedback = "";

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

                        // if (feedback.compare("")) {
                        //     feedback = "Move phone to " + feedback;
                        // } else {
                        //     db_finger_found = 0;
                        //     fingerx = -1;
                        //     fingery = -1;

                        //     previous_fingertip_location[0].x = -1;
                        //     previous_fingertip_location[0].y = -1;
                        //     fingertip_found = 1;
                        //     provide_guidance = 1;
                            
                        //     if (fingertip_not_found % 5 == 0) {
                        //         feedback = "no finger";
                        //         fingertip_not_found++;
                        //     } else {
                        //         feedback = "";
                        //         fingertip_not_found++;
                        //     }          
                        // }
                        
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
                                
                            case 2: 
                            case 3: {
                                // Provide guidance
                                if (guidance_index != -1) {
                                    previous_guidance = guidance;
                                    guidance = getGuidanceForPathMode(button_map, true_labels, button_center[guidance_index], button_shape[guidance_index], fingertip_location, previous_fingertip_location, avg_button_size, guidance_index);
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
                    
                    printf("-- Object Test Point : %f %f \n", fingertip_location[0].x, fingertip_location[0].y);
                    printf("-- Scene Test Point : %f %f \n", scene_fingertip_location[0].x, scene_fingertip_location[0].y);
                    
                    //smooth fingertip location - no need, just filter out unreasonable points
                    //remove rightmost fingertip location - cut side part of reference image solves this problem
                    
                    // drawFingertips(img_results, img_object_vector, curr_state_id, fingertip_location, scene_fingertip_location);
                    cout << "-- Fingertip Location:" << fingertip_location << scene_fingertip_location << endl;
                    
                }
            }

            clock_t loopFeedbackClock = clock();
            double loopMatchingFeedbackDurationClock = (double)(loopFeedbackClock - loopMatchingClock) / CLOCKS_PER_SEC;
            if (file_timer.is_open()) {
                file_timer << loopMatchingFeedbackDurationClock << ",";
            }
            printf("Loop matching-feedback: %f seconds\n", loopMatchingFeedbackDurationClock);
            
            imshow( "Overall Results", img_results );

            // anounceFeedback(feedback);
            
            cout << "Feedback: " << feedback << endl;

            // ****** END CLOCK ******

            // Note: Need to debug this and add this back
            updateImage(db_object_found, db_finger_found, fingerx, fingery);
            idle = 1;

            clock_t loopEndClock = clock();
            double totalDurationClock = (double)(loopEndClock - loopStartClock) / CLOCKS_PER_SEC;
            if (file_timer.is_open()) {
                file_timer << totalDurationClock << endl;
            }
            printf("Overall: %f seconds\n", totalDurationClock);
            
            // Wait to let the iOS end announce state
            if (announcing_state) {
                cout << "Sleep for announcing_state" << endl;
                // sleep(1.5); // 1.5 second
            }

            if (feedback.find("press") != std::string::npos) {
                cout << "Sleep for press" << endl;
                // sleep(1.5); // 1.5 second
            }

            // Technical Evaluation: Store current scene to log folder
            string scene_filename = log_dir + "pic-" + to_string((double)initializeStartClock) + "-" + uniqueFilename(frame) + "-" + to_string(curr_state_id)  + ".jpg";
            imwrite(scene_filename, img_scene);

            waitKey(1); // waits to display frame
        } catch (Exception &e) {
            idle = 1;
            skipImage(); // To be added when sqlite is back
            continue;
        }
    }

    file_trace.close();
    file_timer.close();
    
    waitKey(0);
    return 0;
}

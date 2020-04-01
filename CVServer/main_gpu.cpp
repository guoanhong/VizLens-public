// #include <tesseract/baseapi.h>
// #include <leptonica/allheaders.h>
#include <sys/types.h>

// #include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
// #include <json/json.h>
// #include <json/writer.h>
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
#include <unordered_set>

#include <cstdlib>
#include <cerrno>

#include "curl/curl.h"
// #include "curl/curl_easy.h"

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

#include "db_utils.hpp"
#include "img_utils_gpu.hpp"
// #include "lib/ocr_utils.hpp"
#include "statemap_utils.hpp"
#include "feedback_utils.hpp"
#include "visualization.hpp"

// TODO: define envpaths
// #define PATH genenv("APP_PATH")
#define DB "/home/guoanhong/Desktop/VizLensDynamic/WebServer/db/VizLensDynamic.db"
#define imageDirBase "/home/guoanhong/Desktop/VizLensDynamic/WebServer/images_video/"
#define dirBase "/home/guoanhong/Desktop/VizLensDynamic/CVServer/samples/"
#define session_name "coffee_machine"

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
string mode = "0";
string target = "";
string state = "";
string feedback = "";
string guidance = "";
int idle = 1;
int curr_state_id = -1;

void readme();
int skipFrames(VideoCapture capture, int num);

bool hasEnding (std::string const &fullString, std::string const &ending) {
    if (fullString.length() >= ending.length()) {
        return (0 == fullString.compare (fullString.length() - ending.length(), ending.length(), ending));
    } else {
        return false;
    }
}

// bool NotAlphaNumerical(char c) {
//     return !(('0' <= c && c <= '9') || ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z') 
//         || c == ' ' || c == '\n');
// }

/** @function main */
int main(int argc, char** argv)
{
    cout << "Start (dynamic display for GPU)"<< endl;

    string folder = "hcii_coffee"; // string(argv[1]);
    string video = "video_coffee_hard.MOV"; // string(argv[2]);
    // int ocr_mode = atoi(argv[2]);
    
    cv::gpu::printShortCudaDeviceInfo(cv::gpu::getDevice());
    
    isOpenDB = ConnectDB();
   
    if (isOpenDB) cout << "Connected Successful" << endl;
    else cout << "connection failed " << endl;
    
    vector<Mat> img_object_vector = vector<Mat>();
    vector<vector<vector<int> > > button_map_vector = vector<vector<vector<int> > >();
    vector<vector<string> > true_labels_vector = vector<vector<string> >();
    vector<vector<vector<double> > > button_center_vector = vector<vector<vector<double> > >();
    vector<double> avg_button_size_vector = vector<double>();
    
    vector<GpuMat> img_object_gpu_vector = vector<GpuMat>();
    vector<GpuMat> keypoints_object_gpu_vector = vector<GpuMat>();
    vector<GpuMat> descriptors_object_gpu_vector = vector<GpuMat>();
    
    vector<vector<KeyPoint> > keypoints_object_vector = vector<vector<KeyPoint> >();
    vector<vector<float> > descriptors_object_vector = vector<vector<float> >();

    // vector<string> ocr_object_vector = vector<string>();

    /******************** Read Images ********************/

    // JUDY
    
    int interface_id = getInterfaceId();
    string interface_name = getInterfaceName(interface_id);
    
    // NOTE: in user testing, can either let user input/choose the current interface,
    // or detect the current interface and let the user confirm.
    
    // string dir = string(refImageDirBase) + string(session) + "/";
    string dir = string(dirBase) + folder + "/images/"; // + interface_name + "/";
    string dir_server = "/home/guoanhong/Desktop/VizLensDynamic/WebServer/images_reference/" + string(session_name) + "/";
    
    vector<string> objectnames = vector<string>();
    vector<string> filenames = vector<string>();
    vector<string> files = vector<string>();

    vector<string> description_text_vector = vector<string>();
    
    vector<int> states = vector<int>();
    
    vector<set<int>> transition_graph = vector<set<int>>();
    vector<set<int>> reachable_states = vector<set<int>>();
    
    vector<int> all_ids = vector<int>();

    vector<Mat> new_img_pool;
    vector<vector<KeyPoint> > new_keypoints_pool = vector<vector<KeyPoint> >();
    vector<vector<float> > new_descriptors_pool = vector<vector<float> >();

    vector<GpuMat> new_img_pool_gpu;
    vector<GpuMat> new_keypoints_pool_gpu = vector<GpuMat>();
    vector<GpuMat> new_descriptors_pool_gpu = vector<GpuMat>();

    // vector<string> new_ocr_vector = vector<string>();

    // /* Initialize screenmap visualization */
    // Mat state_graph_pic = Mat::zeros( 250, 750, CV_8UC3 );
    
    // getdir(dir,filenames);

    string read_mode = "db"; // = "local";
    getRefImageFilenames(folder, dir_server, filenames, read_mode);
    sort(filenames.begin(), filenames.end());

    // TESTING
    // return 0;

    // for (int i = 0; i < filenames.size(); i++) {
    //     cout << filenames[i] << "\n";
    // }
    
    ifstream inFile;
    inFile.open((dir_server + "log/state_map.txt").c_str());
    string content((istreambuf_iterator<char>(inFile)), (istreambuf_iterator<char>()));
    inFile.close();
    cout << "state map : " << content << endl;
    getTransitionGraph(content, transition_graph);
    initializeObjects(transition_graph, reachable_states);

    // ifstream inFileOCR;
    // inFileOCR.open(dir + "../log/ocr.txt");
    // string line;
    // while (inFileOCR >> curr_ocr) {
    //     ocr_object_vector.push_back(curr_ocr);
    // }
    // while (getline(inFileOCR, line))
    // {
    //     ocr_object_vector.push_back(line);
    // }

    // cout << ocr_object_vector.size() << endl;
    // // process pair (a,b)
    // inFileOCR.close();


    // /* Initial visualization preparation */
    // drawInitGraph(state_graph_pic, transition_graph);
    
    string ending = "jpg";
    
    int prev_state_id = -1;
    // curr_state_id = -1;

    // NEEDED
//    SURF surf;
//    surf.hessianThreshold = 400;
    // int minHessian = 400;
    // SurfFeatureDetector detector(minHessian);

    SURF_GPU surf;
    surf.hessianThreshold = 400;
    
    // ORIGINAL: READ IN REFERENCE IMAGES
    int id_count = 0;
    for (unsigned int i = 0;i < filenames.size();i++) {
        if (hasEnding (filenames[i], ending)) {
            id_count++;
            cout <<  filenames[i] << endl;
            Mat img_object = imread(dir_server+filenames[i]);
            cout << dir_server+filenames[i] << endl;
            imshow("", img_object);
            
            files.push_back(filenames[i]);
            
            double ratio = img_object.rows > img_object.cols ? 600.0 / (double)img_object.rows : 600.0 / (double) img_object.cols;
            // cout << img_object.shape << endl;

            Size img_object_size(img_object.cols * ratio, img_object.rows * ratio); //(150, 445)
            resize(img_object, img_object, img_object_size);

            img_object_vector.push_back(img_object);
            
            vector<vector<int> > button_map;
            vector<string> true_labels;
            vector<vector<double> > button_center;
            double avg_button_size;
            labels_hash(string(session_name), id_count - 1, img_object, ratio, button_map, true_labels, button_center, avg_button_size);
            // labels_hash(filenames[i].substr(0, filenames[i].size()- 4), img_object, ratio, button_map, true_labels, button_center, avg_button_size);

            button_map_vector.push_back(button_map);
            true_labels_vector.push_back(true_labels);
            button_center_vector.push_back(button_center);
            avg_button_size_vector.push_back(avg_button_size);
            
            cout << dir+filenames[i] << endl;
            lightingDetection(img_object);
            
            // Computer SURF keypoints and descriptors for object (reference) image
            GpuMat img_object_gpu;
            Mat img_object_gray;
            cvtColor(img_object, img_object_gray, CV_RGB2GRAY);
            img_object_gpu.upload(img_object_gray);
            
            GpuMat keypoints_object_gpu;
            GpuMat descriptors_object_gpu;
            // ORIGINAL
            surf(img_object_gpu, GpuMat(), keypoints_object_gpu, descriptors_object_gpu);
            
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
            
            img_object_gpu_vector.push_back(img_object_gpu);
            printf("-- Keypoints Vector Size : %lu \n", keypoints_object_gpu.cols );

            // downloading objects results
            vector<KeyPoint> keypoints_object;
            vector<float> descriptors_object;
            
            // ORIGINAL
            surf.downloadKeypoints(keypoints_object_gpu, keypoints_object);
            surf.downloadDescriptors(descriptors_object_gpu, descriptors_object);
            
            all_ids.push_back(id_count - 1);
            objectnames.push_back(filenames[i].substr(0, filenames[i].size() - 4));
            keypoints_object_vector.push_back(keypoints_object);
            descriptors_object_vector.push_back(descriptors_object);
            keypoints_object_gpu_vector.push_back(keypoints_object_gpu);
            descriptors_object_gpu_vector.push_back(descriptors_object_gpu);
        }
    }

    for (int i = 0; i < files.size(); ++i){
        cout << files[i] << endl;
    }
    
    // TessBaseAPI *tess = new TessBaseAPI();
    // // Initialize tesseract-ocr with English, without specifying tessdata path
    // if (tess->Init(NULL, "eng")) {
    //     fprintf(stderr, "Could not initialize tesseract.\n");
    //     exit(1);
    // }
    // tess->SetVariable("tessedit_char_whitelist", "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ?");
    // unordered_set<string> dictionary;
    // init_dictionary(dictionary);


    GpuMat keypoints_scene_gpu;
    GpuMat descriptors_scene_gpu;
    
    int count = 0;
    clock_t last_time = clock();
    
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
    
    // string video_name = string(dirBase) + folder + "/videos/" + video; // string(imageDirBase) + "video_coffee_hard.mov";
    // VideoCapture capture(video_name);
    // // capture.open(0); // Capture from camera
    // if (!capture.isOpened()) {
    //     throw "--(!) Error reading steam";
    // }
    // int frame = 0;
    // int first_skip_frames = 0;
    // for (int i = 0; i < first_skip_frames; i++) { capture.grab(); }
    // frame += first_skip_frames;

    // Mat img_results = Mat::zeros( 750, 1200, CV_8UC3 );


    if (hasReadyToReadObjects()) {
        int new_state;
        string new_name;
        Mat new_object;
        updateCroppedImage(dir_server, new_name, new_state, new_object);
                    
        // Computer SURF keypoints and descriptors for object (reference) image
        GpuMat new_object_gpu;
        Mat new_object_gray;
        cvtColor(new_object_gpu, new_object_gray, CV_RGB2GRAY);
        new_object_gpu.upload(new_object_gray);
        
        GpuMat new_keypoints_object_gpu;
        GpuMat new_descriptors_object_gpu;
        // ORIGINAL
        surf(new_object_gpu, GpuMat(), new_keypoints_object_gpu, new_descriptors_object_gpu);
        
        // // CPU ONLY
        // vector<KeyPoint> new_keypoints_object;
        // Mat new_descriptors_object; // SHOULD BE vector<float>
        // int minHessian = 400;
        // SurfFeatureDetector detector(minHessian);
        // detector.detect(new_object_gpu, new_keypoints_object);
        // printf("-- Keypoints Vector Size : %lu \n", new_keypoints_object.size() );
        // //-- Step 2: Calculate reference descriptors (feature vectors)
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
        vector<vector<int> > button_map;
        vector<string> true_labels;
        vector<vector<double> > button_center;
        double avg_button_size;
        double obj_ratio = new_object.rows > new_object.cols ? 600.0 / (double)new_object.rows : 600.0 / (double) new_object.cols;
        Size object_size(new_object.cols * obj_ratio, new_object.rows * obj_ratio); //(150, 445)
        resize(new_object, new_object, object_size);
        labels_hash(string(session_name), new_state, new_object, obj_ratio, button_map, true_labels, button_center, avg_button_size);
    
        button_map_vector.push_back(button_map);
        true_labels_vector.push_back(true_labels);
        button_center_vector.push_back(button_center);
        avg_button_size_vector.push_back(avg_button_size);
    }

    for (int i = 0; i < all_ids.size(); i++) {
        description_text_vector.push_back(getStateDescription(i));
    }
    
    int i;
    id = "0";

    bool enableAddNewRefImage = false;
    for ( ; ; ) {
        try {

            // Scene doesn't read in valid image
            // Update cropped image doesn't change it to read


            string image_to_process;
            // ORIGINAL
            while (idle == 1) {
                if(clock() - last_time > (double)(0.05 * 1000000)) {
                    count++;
                    // cout << "GET IMAGE TO PROCESS" << endl;
                    getImageToProcess(image_to_process);
                    last_time = clock();
                }
            }
            // int frames_skipped = 4;
            Mat img_scene = imread(imageDirBase + string(session_name) + "/" + image_to_process);
            cout << imageDirBase + string(session_name) + "/" + image_to_process << endl;
            cout << "reading img_scene" << endl;
            
            ++i;
            
            // CPU ONLY

            /* Use local videos for image stream */
            // Mat img_scene;
            // int frames_skipped = 4;
            // for (int a = 0;  a < frames_skipped - 1; a++) {
            //     capture.grab();
            // }

            // capture >> img_scene;
            // if (img_scene.empty()) break;
            /* Use local videos for image stream */


            // Output scenes for debugging
            
            if (img_scene.empty()) {
                idle = 1;
                skipImage();
                continue;
            }
            double scene_ratio = img_scene.rows > (double)img_scene.cols / 3 * 2 ? 500 / (double)img_scene.rows : 750 / (double)img_scene.cols;
            // img_scene.rows > img_scene.cols ? 600.0 / (double)img_scene.rows : 600.0 / (double) img_scene.cols;
            Size img_scene_size(img_scene.cols * scene_ratio, img_scene.rows * scene_ratio); //(150, 445)
            resize(img_scene, img_scene, img_scene_size);
            
            // imshow("", img_scene);
            
            // frame += frames_skipped;
            
            
            
            // Size img_scene_size(360, 640);
            
            
            
            // cout << "--Size of frame is: "<< img_scene.cols << " " << img_scene.rows << endl;
            
            /* lighting -- 
            lightingDetection(img_scene);
            */

            // isStateProcessing(0);
            
            /****** START CLOCK *******/
            clock_t tic = clock();
            
            // Computer SURF keypoints and descriptors for scene (video) image
            GpuMat img_scene_gpu;
            Mat img_scene_gray;
            cvtColor(img_scene, img_scene_gray, CV_RGB2GRAY);
            // NEEDED
            img_scene_gpu.upload(img_scene_gray);
            surf(img_scene_gpu, GpuMat(), keypoints_scene_gpu, descriptors_scene_gpu);
            
            // // CPU ONLY
            // vector<KeyPoint> keypoints_scene;
            // Mat descriptors_scene; // SHOULD BE vector<float>
            // detector.detect(img_scene_gpu, keypoints_scene);
            // // printf("-- Keypoints Vector Size : %lu \n", keypoints_scene.size() );
            // //-- Step 2: Calculate reference descriptors (feature vectors)
            // SurfDescriptorExtractor extractor;
            // extractor.compute(img_scene_gpu, keypoints_scene, descriptors_scene);
            
            cout << "FOUND " << keypoints_scene_gpu.cols << " keypoints on second image" << endl;

            if (keypoints_scene_gpu.empty()) {
                idle = 1;
                skipImage();
                continue;
            }

            // downloading scene results
            vector<KeyPoint> keypoints_scene;
            vector<float> descriptors_scene;
            surf.downloadKeypoints(keypoints_scene_gpu, keypoints_scene);
            surf.downloadDescriptors(descriptors_scene_gpu, descriptors_scene);
            
            /******************** Start Matching ********************/

            vector< DMatch > good_matches;
            int state_index = 0;

            Mat H;
            int inlier_max = -1;

            float ratio_max = 0;
            
            // THE ORIGINAL MATCHING PROCESS
            // for (int i = 0;i < files.size();i++) {
            // ....
            // }

            // // NEW MATCHING WITH THE "STEP" FILTER
            // vector<Mat> reachable_descriptors_object_gpu_vector;
            // if (prev_state_id >= 0) {
            //     reachable_descriptors_object_gpu_vector = vector<Mat>();
            //     filterObjects(prev_state_id, reachable_states, reachable_descriptors_object_gpu_vector, descriptors_object_gpu_vector);
            // } else {
            //     reachable_descriptors_object_gpu_vector = descriptors_object_gpu_vector;
            // }
            
            // vector<Mat> reachable_img_object_vector;
            // if (prev_state_id >= 0) {
            //     reachable_img_object_vector = vector<Mat>();
            //     filterOriginalImgs(prev_state_id, reachable_states, reachable_img_object_vector, img_object_vector);
            // } else {
            //     reachable_img_object_vector = img_object_vector;
            // }
            
            // vector<int> reachable_ids = all_ids;
            // if (prev_state_id >= 0) {
            //     reachable_ids.resize(reachable_states[prev_state_id].size());
            //     copy(reachable_states[prev_state_id].begin(), reachable_states[prev_state_id].end(), reachable_ids.begin());
            // }
            
            cout << "------------------------------------------\n";
            cout << "Start Matching \n";
            cout << "------------------------------------------\n";


            for (int i = 0;i < descriptors_object_gpu_vector.size();i++) {
            // for (int i = 0;i < reachable_descriptors_object_gpu_vector.size();i++) {
                // matching descriptors
                BFMatcher_GPU matcher(NORM_L2);
                GpuMat trainIdx, distance;
                
                // // CPU ONLY
                // vector<DMatch> matches;
                // // matcher.match(reachable_descriptors_object_gpu_vector[i], descriptors_scene, matches);
                // matcher.match(descriptors_object_gpu_vector[i], descriptors_scene, matches);

                matcher.matchSingle(descriptors_object_gpu_vector[i], descriptors_scene_gpu, trainIdx, distance);
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
                // if (i == 0 || inliersCount > inlier_max) {
                //     inlier_max = inliersCount;
                //     state_index = i;
                //     H = tempH;
                // }

                float currRatio = inliersCount / (float)tmp_good_matches.size(); //  / ((float)keypoints_object_vector[i].size() + (float)keypoints_scene.size());
                cout << "Ratio of Inlier: " << currRatio << endl;
                cout << endl;
                if (i == 0 || currRatio > ratio_max) {
                    ratio_max = currRatio;
                    inlier_max = inliersCount;
                    state_index = i;
                    H = tempH;
                }

            }


            /******************** OCR ********************/

            
            // Mat img_scene_ocr;
            string text_scene;

            float ratio_lower = 0.5;
            float ratio_upper = 0.7;

            if (ratio_max < 0.7) {
            	enableAddNewRefImage = false;
            /*
            if (ratio_lower < ratio_max && ratio_max < ratio_upper) {
                
                text_scene = ocr_find_text(tess, img_scene_gray, dictionary);
                cout << "OCR Output: " << text_scene << endl;
            }
            string text_object = ocr_object_vector.size() == 0 ? "" : ocr_object_vector[state_index];
            float ocr_score = ocr_similarity_score(text_scene, text_object);
 
            int len_short = text_object.length() < text_scene.length() ? text_object.length() : text_scene.length();
            int len_long = text_object.length() + text_scene.length() - len_short;
            float ocr_ratio_short = ocr_score / len_short;
            float ocr_ratio_long = ocr_score / len_long;
            if (ratio_lower < ratio_max && ratio_max < ratio_upper) {
                cout << "OCR Ratio: " << ocr_ratio_short << ", " << ocr_ratio_long << endl;
            }
            bool ocr_not_found = text_object.length() == 0 || text_scene.length() == 0
                || ocr_ratio_short <= 0.6 || ocr_ratio_long <= 0.5;
            */
            /******************** ADD NEW REF IMAGE ********************/
            
            // IF NOT FOUND, ADD IMAGE TO REFERENCE IMAGEm
            // int inlierThreshold = 20;
            // float ratioThreshold = 0.7;
            // float ocrThreashold = 0.7;

            /*
            if (ratio_lower < ratio_max && ratio_max < ratio_upper) {
                cout << "-- OCR SCORE --" << endl;
                cout << ocr_score << endl;
            }
            

            cout << "-- MAX RATIO --" << endl;
            cout << ratio_max << endl;
            cout << endl;

            if (ratio_max <= ratio_lower || (ratio_lower < ratio_max && ratio_max < ratio_upper && ocr_not_found)) {
            */
    
                if (!enableAddNewRefImage) continue;
            // if (ratio_max < ratioThreshold) {
            // if (ratio_max <= 0.5 || (0.5 < ratio_max && ratio_max < 0.8 && ocr_score < ocrThreashold)) { // (inlier_max / (float)keypoints_scene.size() < ratioThreshold) {

                cout << "-- FOUND UNMATCHED IMAGE -- \n";
                new_img_pool.push_back(img_scene);
                new_keypoints_pool.push_back(keypoints_scene);
                new_descriptors_pool.push_back(descriptors_scene);

                new_img_pool_gpu.push_back(img_scene_gpu);
                new_keypoints_pool_gpu.push_back(keypoints_scene_gpu);
                new_descriptors_pool_gpu.push_back(descriptors_scene_gpu);

                bool matched = checkNewImgFromPool(new_img_pool, new_img_pool_gpu, new_keypoints_pool, new_keypoints_pool_gpu, new_descriptors_pool, new_descriptors_pool_gpu,
                    img_scene_gpu, keypoints_scene, descriptors_scene_gpu);

                if (matched) {
                    cout << "UNMATCHED IMAGE MATCHED IN POOL \n";
                    Mat img_object = img_scene;

                    img_object_vector.push_back(img_object);
                    img_object_gpu_vector.push_back(img_scene_gpu);

                    cout << "-- ADDING NEW REFERENCE IMAGE -- \n";
                    state_index = all_ids.size();
                    cout << "state_index = " << state_index << endl; //  + ", Ratio = " + to_string(inlier_max / (float)keypoints_scene.size()) + "\n";

                    string new_name = state_index < 10 ? "New0" + to_string(state_index) + ".jpg" : "New" + to_string(state_index) + ".jpg";
                    string new_file = dir + new_name;

                    sendTransitionInstructions("Adding new state, please wait.");

                    if (read_mode == "local") {
                        // files.push_back(new_name);
                        imwrite(new_file, img_object);
                        imwrite("../WebServer/images_original/" + string(session_name) + "/" + new_name, img_object);

                    } else if (read_mode == "db") {  

                        // if (!isStateProcessing(state_index)) {

                        cout << "HTTP request" << endl;
                        // struct stat file_info;
                        // FILE *fd;
                        // fd = fopen("/Users/judy/Desktop/VizLens/VizLensDynamic/WebServer/images_original/New00.jpg", "r"); 
                        // fstat(fileno(fd), &file_info);

                        // if(!fd)
                        //     return 1; /* can't continue */ 
                        // /* to get the file size */ 
                        // if(fstat(fileno(fd), &file_info) != 0)
                        //     return 1; /* can't continue */ 

                        imwrite(new_file, img_object);
                        imwrite("../WebServer/images_original/" + string(session_name) + "/" + new_name, img_object);

                        CURL *hnd = curl_easy_init();

                        curl_easy_setopt(hnd, CURLOPT_CUSTOMREQUEST, "POST");
                        string url = "http://localhost:8888/VizLensDynamic/php/uploadOriginalforCVServer.php?userid=testuser&session=" + string(session_name) + "&stateid=" + to_string(state_index);
                        curl_easy_setopt(hnd, CURLOPT_URL, url.c_str());
                        struct curl_slist *headers = NULL;
                        // headers = curl_slist_append(headers, "postman-token: 92191e21-8ce2-ff44-9c98-8cfd401b902f");
                        headers = curl_slist_append(headers, "cache-control: no-cache");
                        headers = curl_slist_append(headers, "content-type: multipart/form-data; boundary=----WebKitFormBoundary7MA4YWxkTrZu0gW");
                        curl_easy_setopt(hnd, CURLOPT_HTTPHEADER, headers);
                        string upload_file = "------WebKitFormBoundary7MA4YWxkTrZu0gW\r\nContent-Disposition: form-data; name=\"pictures[0]\"; filename=\"" + new_name + "\"\r\nContent-Type: image/jpeg\r\n\r\n\r\n------WebKitFormBoundary7MA4YWxkTrZu0gW--";
                        curl_easy_setopt(hnd, CURLOPT_POSTFIELDS, upload_file.c_str());

                        // curl_easy_setopt(hnd, CURLOPT_READDATA, fd);
                        // curl_easy_setopt(hnd, CURLOPT_INFILESIZE_LARGE,(curl_off_t)file_info.st_size);
                        // curl_easy_setopt(hnd, CURLOPT_VERBOSE, 1L);

                        CURLcode ret = curl_easy_perform(hnd);

                        /*
                        istringstream myStream(dir + new_name + ".jpg");
                        int size = myStream.str().size();  
                        char buf[50];   
                        char *url = (char*)"http://localhost:8888/VizLensDynamic/php/uploadOriginal.php?userid=testuser&session=testsession&stateid=0";
                        try {
                            curlpp::Cleanup cleaner;
                            curlpp::Easy request;
                            using namespace curlpp::Options;

                            request.setOpt(new Url(url));
                            // request.setOpt(new Verbose(true));

                            list< string > headers;
                            headers.push_back("cache-control: no-cache");
                            headers.push_back("Content-Type: multipart/form-data; boundary=----WebKitFormBoundary7MA4YWxkTrZu0gW"); 
                            
                            // sprintf(buf, "Content-Length: %d", size); 
                            // headers.push_back(buf);

                            curlpp::Forms formParts;
                            formParts.push_back(new curlpp::FormParts::Content("Content-Type", "image/jpeg"));
                            formParts.push_back(new curlpp::FormParts::Content("pictures[0]", "@"+ dir + new_name + ".jpg")); 
                            // formParts.push_back(new curlpp::FormParts::Content("filename", )); 
                            // formParts.push_back(new curlpp::FormParts::Content("picture[0]", new_name + ".jpg"));
                            request.setOpt(new curlpp::options::HttpPost(formParts)); 

                            // request.setOpt(new curlpp::options::Verbose(true)); 
                            
                            // request.setOpt(new InfileSize(size));
                            // request.setOpt(new Upload(true));
                            request.setOpt(new HttpHeader(headers));
                            
                            request.perform(); 

                            cout << "Upload to database successful: " << state_index << endl;
                            return 0;
                        }
                        catch ( curlpp::LogicError & e ) {
                            std::cout << e.what() << std::endl;
                        }
                        catch ( curlpp::RuntimeError & e ) {
                            std::cout << e.what() << std::endl;
                        }
                        */

                        // }
                    }


                    /* Add OCR */
                    // if (ratio_max <= 0.3) {
                        // text_scene = ocr_find_text(tess, img_scene_gray, dictionary);
                        // cout << "OCR Output: " << text_scene << endl;
                    // }
                    
                    // TODO: Might need to change text_scene to updated OCR? Or not important?
                    // ocr_object_vector.push_back(text_scene);
                    addStateToTransitionGraph(interface_id, transition_graph, state_index);


                    string out_string = storeTransitionGraph(transition_graph);
                    ofstream file(dir_server + "log/state_map.txt");
                    if (file.is_open()) {
                        file << out_string;
                    }
                    file.close();

                    // ofstream file_ocr(dir + "../log/ocr.txt");
                    // if (file_ocr.is_open()) {
                    //     for (int i = 0; i < ocr_object_vector.size(); i++) {
                    //         file_ocr << ocr_object_vector[i] << endl;
                    //     }
                    // }
                    // file_ocr.close();


                    // TODO: WAIT; PULL REFERENCE IMAGE FROM DB; UPDATE IMG_OBJECT
                    updateCroppedImage(dir_server, new_name, state_index, img_object);
                    enableAddNewRefImage = false;

                    files.push_back(new_name);

                    all_ids.push_back(state_index);
                    objectnames.push_back(new_name);

                    // Can add this line to store image locally
                    
                    // TODO: re-detect keypoints & descriptors
                    keypoints_object_vector.push_back(keypoints_scene);
                    descriptors_object_vector.push_back(descriptors_scene);
                    keypoints_object_gpu_vector.push_back(keypoints_scene_gpu);
                    descriptors_object_gpu_vector.push_back(descriptors_scene_gpu);

                    // VizDynamic: May need crowdsourcing for labeling buttons in added image object
                    vector<vector<int> > button_map;
                    vector<string> true_labels;
                    vector<vector<double> > button_center;
                    double avg_button_size;
                    double obj_ratio = img_object.rows > img_object.cols ? 600.0 / (double)img_object.rows : 600.0 / (double) img_object.cols;
                    Size object_size(img_object.cols * obj_ratio, img_object.rows * obj_ratio); //(150, 445)
                    resize(img_object, img_object, object_size);
                    labels_hash(string(session_name), state_index, img_object, obj_ratio, button_map, true_labels, button_center, avg_button_size);
                
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
                    description_text_vector.push_back(getStateDescription(state_index));
                    sendTransitionInstructions("New state ready, please resume.");

                } else {
                    continue;
                }
            }
            
            // Use the history of detected screens to determine the current screen and get rid of noises.
            // By JUDY
            if (states.size() == 6) {
                states.erase(states.begin());
            }
            states.push_back(state_index);

            string matched_state;
            string current_state;
            int new_state_id = findCurrentState(states);
            if (new_state_id != curr_state_id) {
                prev_state_id = curr_state_id;
                curr_state_id = new_state_id;

                // string description_text = "";
                // sendStateDescription(curr_state_id);
                // feedback = description_text;
            }
            cout << "Current state id: "<< curr_state_id <<endl;

            enableAddNewRefImage = true;

            // cout << transition_graph.size() << endl;
            updateTransitionGraph(interface_id, transition_graph, prev_state_id, curr_state_id);

            cout << "Transition graph updated" << endl;
            string out_string = storeTransitionGraph(transition_graph);
            ofstream file(dir_server + "log/state_map.txt");
            if (file.is_open()) {
                file << out_string;
            }
            file.close();
            
            cout << "PREVIOUS STATE: " << prev_state_id << endl;
            cout << "CURRENT STATE: " << curr_state_id << " - " << current_state << endl;
            
            


            /******************** Send Feedback ********************/


            // NEED TO FIX state_index

            // cout << img_object_vector.size() << "," << button_map_vector.size() << "," << true_labels_vector.size() << "," << button_center_vector.size() << "," << avg_button_size_vector.size() << endl; 
            Mat img_object = img_object_vector[curr_state_id];
            vector<vector<int> > button_map = button_map_vector[curr_state_id];
            vector<string> true_labels = true_labels_vector[curr_state_id];
            vector<vector<double> > button_center = button_center_vector[curr_state_id];
            double avg_button_size = avg_button_size_vector[curr_state_id];

            // double object_ratio = img_scene.rows > img_object.cols ? 200.0 / (double)img_object.rows : 200.0 / (double) img_object.cols;
            // Size img_object_size(img_object.cols * object_ratio, img_object.rows * object_ratio); //(150, 445)
            // resize(img_object, img_object, img_object_size);
            
//            img_object.copyTo(img_results(Rect(0,0,img_object.cols, img_object.rows)));
//            img_scene.copyTo(img_results(Rect(img_object.cols,0,img_scene.cols, img_scene.rows)));
            
            
//            drawMatches( img_object, keypoints_object_vector[state_index], img_scene, keypoints_scene_gpu,
//                        good_matches, img_results, Scalar::all(-1), Scalar::all(-1),
//                        vector<char>(), DrawMatchesFlags::NOT_DRAW_SINGLE_POINTS );
            
//            Mat img_results;
//            imshow("",img_scene);
//            imshow("", img_object);

            // DRAW MAT
            Mat img_results = Mat::zeros( 750, 1200, CV_8UC3 );
            drawScreenMap(img_results, img_scene, img_object_vector, curr_state_id, all_ids);
            //// drawScreenMap(img_results, imgm_scene, reachable_img_object_vector, curr_state_id, reachable_ids);
            drawStateGraph(img_results, transition_graph, curr_state_id);
            displayLogOnScreen(img_results, interface_name, objectnames, states, matched_state, current_state, "");// ocr_object_vector[curr_state_id]);

            

            // FINGERTIP LOCATION

            // Get the corners from the object (reference) image
            vector<Point2f> object_corners(5);
            getImageCorners(img_object, object_corners);
            
            // Transform object corners to scene corners using H
            vector<Point2f> scene_corners(5);
            // Might be better to user perspectiveTransform, use direct ratio for faster speed and testing
            // float ratio = img_scene.cols / (float)img_object.cols;
            // for (int i = 0; i < 5; i++) scene_corners[i] = object_corners[i] * ratio;
            perspectiveTransform( object_corners, scene_corners, H);
//            cout << "Image corners: " << scene_corners[1] << scene_corners[2] << scene_corners[3] << scene_corners[4] << endl;
            


            int guidance_index = -2;
            

            // Set mode to -2 for testing

            if (stoi(mode) == 2) {
                guidance_index = getGuidanceIndex(true_labels, target);
            }
            

            vector<Point2f> fingertip_location(1);
            
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
                    
                    // // If found, draw rectangles around both images
                    // drawRects(img_results, object_corners, scene_corners, img_object);
                    
                    // Warp scene image with finger to reference image size
                    Mat img_skin;
                    warpPerspective(img_scene, img_skin, H.inv(), img_object.size(), INTER_LINEAR, BORDER_CONSTANT);
                    // printf("Elapsed: %f seconds\n", (double)(toc - tic) / CLOCKS_PER_SEC);

                    // Filter skin color to get binary image
                    Mat img_skin_binary;
                    // skinColorAdjustment(img_object, img_skin);
                    skinColorFilter(img_skin, img_skin_binary);
                    // skinColorSubtractor(img_skin, img_object, img_skin_binary);
                    
                    // Find fingertip location in reference image
                    fingertip_location[0].x = -1;
                    fingertip_location[0].y = -1;
                    findFingertipLocation(img_skin_binary, img_object.rows * img_object.cols, H, avg_button_size, fingertip_location);

                    // Find relative fingertip location in scene image
                    vector<Point2f> scene_fingertip_location(1);
                    perspectiveTransform( fingertip_location, scene_fingertip_location, H);
                    
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
                        
                        // Disable feedback for testing 

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
                    
                    printf("-- Object Test Point : %f %f \n", fingertip_location[0].x, fingertip_location[0].y);
                    printf("-- Scene Test Point : %f %f \n", scene_fingertip_location[0].x, scene_fingertip_location[0].y);
                    
                    //smooth fingertip location - no need, just filter out unreasonable points
                    //remove rightmost fingertip location - cut side part of reference image solves this problem
                    
                    drawFingertips(img_results, img_object_vector, curr_state_id, fingertip_location, scene_fingertip_location);
                    cout << "-- Fingertip Location:" << fingertip_location << scene_fingertip_location << endl;
                    
                    if (state.compare(current_state)) {
                        string currfeedback = description_text_vector[curr_state_id];
                        feedback = "State: " + currfeedback + " " + feedback;
                        // getStateDescription(curr_state_id)
                        state = current_state;
                    }
                    
                }
            }

            
            
            imshow( "Overall Results", img_results );

            // anounceFeedback(feedback);
            
            
            cout << "Feedback: " << feedback << endl;
            clock_t toc = clock();
            printf("Elapsed: %f seconds\n", (double)(toc - tic) / CLOCKS_PER_SEC);

            // ****** END CLOCK ******

            updateImage(db_object_found, db_finger_found, fingerx, fingery);
            idle = 1;
            
            waitKey(1); // waits to display frame
        } catch (Exception &e) {
            idle = 1;
            skipImage(); // To be added when sqlite is back
            continue;
        }
    }

    // Store 
    // storeTransitionGraph(dir, transition_graph);
    // Json::Value value_obj;
    // Json::StreamWriterBuilder builder;
    string out_string = storeTransitionGraph(transition_graph);
    ofstream file(dir_server + "log/state_map.txt");
    if (file.is_open()) {
        file << out_string;
    }
    file.close();

    // ofstream file_ocr(dir + "../log/ocr.txt");
    // if (file_ocr.is_open()) {
    //     for (int i = 0; i < ocr_object_vector.size(); i++) {
    //         file_ocr << ocr_object_vector[i] << endl;
    //     }
    // }
    // file_ocr.close();

    
    waitKey(0);
    return 0;
}




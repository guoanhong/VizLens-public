/* Visualization Headers */

#include <iostream>
#include <set>
#include "opencv2/imgproc/imgproc.hpp"

using namespace cv;
using namespace std;

void drawScreenMap(Mat img_results, Mat img_scene, vector<Mat> reachable_img_object_vector, int curr_state_id, vector<int> reachable_ids);
void drawStateGraph(Mat img_results, vector<map<int, int>> transition_graph, int curr_state_id);
void displayLogOnScreen(Mat img_results, vector<string> objectnames, vector<int> states, string matched_state, string current_state, string ocr_text);
void drawFingertips(Mat img_results, vector<Mat> img_object_vector, int curr_state_id, vector<Point2f> fingertip_location, vector<Point2f> scene_fingertip_location);
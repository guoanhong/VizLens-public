/* Feedback Utils Headers */

#include <iostream>
#include "opencv2/highgui/highgui.hpp"

using namespace cv;
using namespace std;

string getFeedback(vector<vector<int> > button_map, vector<string> true_labels, vector<Point2f> fingertip_location, vector<Point2f> previous_fingertip_location, double avg_button_size);
int getGuidanceIndex(vector<string> true_labels, string targets);
int getGuidanceIndexBasedOnExpectedPath(vector<int> state_path, vector<int> button_path, vector<map<int, int>> transition_graph, vector<vector<int>> button_id_vector, int curr_state_id, int &next_idx_on_path);
string getGuidance(vector<vector<int> > button_map, vector<string> true_labels, vector<double> guidance_button_center, vector<Point2f> fingertip_location, vector<Point2f> previous_fingertip_location, double avg_button_size);
string getGuidanceForPathMode(vector<vector<int> > button_map, vector<string> true_labels, vector<double> guidance_button_center, vector<double> guidance_button_shape, vector<Point2f> fingertip_location, vector<Point2f> previous_fingertip_location, double avg_button_size, int guidance_index);
string getDirection(vector<double> guidance_button_center, vector<Point2f> fingertip_location, double avg_button_size);
string getDirectionForPathMode(vector<double> guidance_button_center, vector<double> guidance_button_shape, vector<Point2f> fingertip_location);
void anounceFeedback(string feedback);

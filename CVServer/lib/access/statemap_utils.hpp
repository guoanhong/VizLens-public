/* State Map Headers */

#include <iostream>
#include <set>

#include "opencv2/highgui/highgui.hpp"
#include <opencv2/gpu/gpu.hpp>
#include "opencv2/nonfree/gpu.hpp"

using namespace cv;
using namespace std;
using namespace cv::gpu;

int findCurrentState(vector<int> states);
Point2f findTransitionFingertipLocation(int new_id, vector<int> states, vector<Point2f> fingertip_locations);
void getCandidateObjectIds(int curr_state_id, vector<int> all_ids, vector<map<int, int>> transition_graph, vector<int> &candidate_ids_object_vector);
void getCandidateObjectDescriptors(vector<int> candidate_ids_object_vector, vector<Mat> descriptors_object_gpu_vector, vector<Mat> &candidate_descriptors_object_vector);
void getCandidateObjectDescriptorsGPU(vector<int> candidate_ids_object_vector, vector<GpuMat> descriptors_object_gpu_vector, vector<GpuMat> &candidate_descriptors_object_gpu_vector);
void addStateToTransitionGraph(string session_name, vector<map<int, int>> &transition_graph, int new_state_id);
void updateTransitionGraph(string session_name, vector<map<int, int>> &transition_graph, int curr_state_id, int new_state_id, int button_id);
void initializeObjects(vector<set<int>> adjList, vector<set<int>> &reachable_states);
void filterObjects(int prev_state_id, vector<set<int>> reachable_states, vector<Mat> &reachable_descriptors_object_gpu_vector, vector<Mat> descriptors_object_gpu_vector);
void filterOriginalImgs(int prev_state_id, vector<set<int>> reachable_states, vector<Mat> &reachable_img_object_vector, vector<Mat> img_object_vector);
// string storeTransitionGraph(vector<map<int, int>> transition_graph);
int getExpectedNextState(vector<map<int, int>> transition_graph, int curr_state_id, int button_index);


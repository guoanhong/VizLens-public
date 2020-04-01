/* Database Utils Headers */

#include <dirent.h>
#include <iostream>
#include "sqlite3.h"
#include "opencv2/highgui/highgui.hpp"

using namespace cv;
using namespace std;

bool ConnectDB(const char* DB);
void DisonnectDB(const char* DB);
void getExpectedPath(vector<int> &state_path, vector<int> &button_path);
void getRefImageFilenames(string session_name, string dir, vector<string> &filenames, string mode);
void getTransitionGraph(string session_name, string read_mode, vector<string> filenames, vector<map<int, int>> &transition_graph);
void getImageToProcess(string &image_to_process);
int updateImage(int objectfound, int fingerfound, int fingerx, int fingery);
int skipImage();
void sendTransitionInstructions(string session, string instruction);
string getStateDescription(string session, int state_id);
bool hasReadyToReadObjects();
void updateCroppedImage(string session, string dir_server, string &new_name, int &state_id, Mat &img_object);
void labels_hash(string session_name, int state_index, Mat img_object, double ratio, vector<int> &button_id, vector<vector<int> > &button_map, vector<string> &true_labels, vector<vector<double> > &button_center, double &avg_button_size);
void addTransitionToDB(string session_name, int curr_state_id, int new_state_id, int button_id);
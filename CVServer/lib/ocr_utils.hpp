#include <fstream>
#include <iostream>
#include <regex>
#include <string>
#include <unordered_set>
#include <json/json.h>
#include "cpp-base64/base64.h"
#include "curl/curl.h"
#include "opencv2/highgui/highgui.hpp"

using namespace std;
using namespace cv;

void init_dictionary(unordered_set<string> &dictionary);
void getScreenRegionFromImage(Mat img, string &screen_exist, vector<double> &screen_box);
void getOCRTextFromImage(Mat img, string &fulltext);
float ocr_similarity_score(string str1, string str2);
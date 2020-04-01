/* Visualization Implementation */

#include "visualization.hpp"

extern string id;
extern string image;
extern string mode;
extern string target;
extern string state;
extern string feedback;
extern string guidance;
extern int idle;

void drawScreenMap(Mat img_results, Mat img_scene, vector<Mat> reachable_img_object_vector, int curr_state_id, vector<int> reachable_ids) {
    
    img_scene.copyTo(img_results(Rect(450, 250 ,img_scene.cols, img_scene.rows)));
    
    line(img_results, cvPoint(450, 0), cvPoint(450, 750), Scalar(0, 255, 0), 2);
    line(img_results, cvPoint(450, 250), cvPoint(1200, 250), Scalar(0, 255, 0), 2);
    
    int num_ref_img = (int)reachable_img_object_vector.size() > 5 ? (int)reachable_img_object_vector.size() : 5;
    int ref_height = 750 / num_ref_img - 4;
    int ref_width = ref_height * 3 / 2;
    for (int i = 0; i < reachable_img_object_vector.size(); i++) {
        
        Mat curr_object = reachable_img_object_vector[i];
        double ratio = curr_object.rows > curr_object.cols * 2.0 / 3.0 ? ref_height / (double)curr_object.rows : ref_width / (double) curr_object.cols;
        Size curr_object_size(curr_object.cols * ratio, curr_object.rows * ratio); //(150, 445)
        resize(curr_object, curr_object, curr_object_size);
        
        int x0 = 225 - ref_width / 2;
        int y0 = i * (ref_height + 4) + 2;
        curr_object.copyTo(img_results(Rect(x0, y0, curr_object.cols, curr_object.rows)));
        
        if (i == curr_state_id) { // (i < reachable_ids.size() && reachable_ids[i] == curr_state_id) {
            line(img_results, cvPoint(x0, y0), cvPoint(x0 + curr_object.cols, y0), Scalar(0, 255, 0), 2);
            line(img_results, cvPoint(x0 + curr_object.cols, y0), cvPoint(x0 + curr_object.cols, y0 + curr_object.rows), Scalar(0, 255, 0), 2);
            line(img_results, cvPoint(x0 + curr_object.cols, y0 + curr_object.rows), cvPoint(x0, y0 + curr_object.rows), Scalar(0, 255, 0), 2);
            line(img_results, cvPoint(x0, y0 + curr_object.rows), cvPoint(x0, y0), Scalar(0, 255, 0), 2);
        }
        
    }
    
}

void drawStateGraph(Mat img_results, vector<map<int, int>> transition_graph, int curr_state_id) {

    int xOffset = 450;
    int yOffset = 0;

    // DEBUG: print all states in the state diagram
    /*
    for (int i = 0; i < transition_graph.size(); i++) {
        cout << "Neighbors of state " << i << ": ";
        map<int, int>::iterator it;
        for (it = transition_graph[i].begin(); it != transition_graph[i].end(); it++) {
            int state = it->second; // Access value (target state id)
            cout << state << " ";
        }
        cout << endl;
    }
    */

    vector<vector<int>> levels;
    set<int> visited;
    vector<int> currLevel;
    vector<int> prevLevel;

    visited.insert(0);
    currLevel.push_back(0);
    levels.push_back(currLevel);

    while (visited.size() < transition_graph.size() && currLevel.size() > 0) {

        vector<int> tmp;
        prevLevel = currLevel;
        currLevel = tmp;

        for (int i = 0; i < prevLevel.size(); i++) {
            map<int, int> neighbors = transition_graph[prevLevel[i]];
            map<int, int>::iterator it;
            for (it = neighbors.begin(); it != neighbors.end(); it++) {
                int state = it->second; // Access value (target state id)
                if (prevLevel[i] == state) continue;
                if (visited.find(state) == visited.end()) {
                    visited.insert(state);
                    currLevel.push_back(state);
                } 
            }
        }

        sort(currLevel.begin(), currLevel.end());
        levels.push_back(currLevel);

    }

    int radius = 20;
    int margin = 10;
    int gap = 5;
    int colWidth = 75;

    for (int previ = 0; previ < levels.size(); previ++) {
        for (int curri = levels.size() - 1; curri >= previ; curri--) {
            vector<int> prevLevel = levels[previ];
            vector<int> currLevel = levels[curri];
            for (int prevj = 0; prevj < prevLevel.size(); prevj++) {
                for (int currj = 0; currj < currLevel.size(); currj++) {
                    if (previ == curri && prevj == currj) continue;
                    bool found = false;
                    map<int,int>::iterator it;
                    for (it = transition_graph[prevLevel[prevj]].begin(); it != transition_graph[prevLevel[prevj]].end(); ++it) {
                        if (it->second == currLevel[currj]) {
                            found = true;
                        }
                    }
                    if (!found) continue;
                    int x = margin + colWidth * curri + colWidth / 2;
                    int y = margin + (currj * 2 + 1) * radius + (currj + 1) * gap;;
                    int prevx = margin + colWidth * previ + colWidth / 2;
                    int prevy = margin + (prevj * 2 + 1) * radius + (prevj + 1) * gap;
                    if (curri - previ == 1) {
                        line(img_results, cvPoint(xOffset + prevx, yOffset + prevy), cvPoint(xOffset + x, yOffset + y), Scalar(200, 200, 200), 2);
                    } else {
                        line(img_results, cvPoint(xOffset + prevx, yOffset + prevy), cvPoint(xOffset + x, yOffset + y), Scalar(180, 180, 180), 2);
                    }
                }
            }
        }
    }

    for (int i = 0; i < levels.size(); i++) {
        vector<int> currLevel = levels[i];
        for (int j = 0; j < currLevel.size(); j++) {
            int x = margin + colWidth * i + colWidth / 2;
            int y = margin + (j * 2 + 1) * radius + (j + 1) * gap;
            if (currLevel[j] == curr_state_id) {
                circle(img_results, cvPoint(xOffset + x, yOffset + y), radius, Scalar(0, 255, 0), CV_FILLED, 2);
            } else {
                circle(img_results, cvPoint(xOffset + x, yOffset + y), radius, Scalar(255, 255, 255), CV_FILLED, 2);
            } 
            if (currLevel[j] <= 9) {
                putText(img_results, to_string(currLevel[j]), cvPoint(xOffset + x - radius / 2 + 3, yOffset + y + radius / 2 - 1), FONT_HERSHEY_PLAIN, 1.5, Scalar(0, 0, 0), 2);
            } else {
                putText(img_results, to_string(currLevel[j]), cvPoint(xOffset + x - radius / 2 - 5, yOffset + y + radius / 2 - 1), FONT_HERSHEY_PLAIN, 1.5, Scalar(0, 0, 0), 2);
            }
        }
    }
}

void displayLogOnScreen(Mat img_results, vector<string> objectnames, vector<int> states, string matched_state, string current_state, string ocr_text) {
    int xOffset = 450 + 20;
    int yOffset = 0 + 130;
    int margin = 10;
    int row = 20;

    putText(img_results, "Text: " + ocr_text, cvPoint(xOffset + margin, yOffset + margin + row * 1 + row / 2), FONT_HERSHEY_SIMPLEX, 0.5, Scalar(255, 255, 255), 1);
    string state_smoother;
    bool flag = false;
    for (int i = 0; i < states.size() - 1; i++) {
        state_smoother += objectnames[states[i]] + ", ";
        if (state_smoother.length() > 50) {
            putText(img_results, "State Smoother: " + state_smoother, cvPoint(xOffset + margin, yOffset + margin + row * 2 + row / 2), FONT_HERSHEY_SIMPLEX, 0.5, Scalar(255, 255, 255), 1);
            state_smoother = "";
            flag = true;
        }
    }
    if (states.size() > 0) state_smoother += objectnames[states[states.size() - 1]]; // files[states[states.size() - 1]].substr(0, files[states[states.size() - 1]].size() - 9);
    if (flag) {
        putText(img_results, "                " + state_smoother, cvPoint(xOffset + margin, yOffset + margin + row * 3 + row / 2), FONT_HERSHEY_SIMPLEX, 0.5, Scalar(255, 255, 255), 1);
    } else {
        putText(img_results, "State Smoother: " + state_smoother, cvPoint(xOffset + margin, yOffset + margin + row * 2 + row / 2), FONT_HERSHEY_SIMPLEX, 0.5, Scalar(255, 255, 255), 1);
    }
    putText(img_results, "Matched State: " + matched_state, cvPoint(xOffset + margin, yOffset + margin + row * 3 + row / 2), FONT_HERSHEY_SIMPLEX, 0.5, Scalar(255, 255, 255), 1);
    putText(img_results, "Current State: " + current_state, cvPoint(xOffset + margin, yOffset + margin + row * 4 + row / 2), FONT_HERSHEY_SIMPLEX, 0.5, Scalar(255, 255, 255), 1);
    // putText(img_results, "OCR: " + ocr_text, cvPoint(xOffset + margin, yOffset + margin + row * 3 + row / 2), FONT_HERSHEY_SIMPLEX, 0.5, Scalar(255, 255, 255), 1);
}


void drawFingertips(Mat img_results, vector<Mat> img_object_vector, int curr_state_id, vector<Point2f> fingertip_location, vector<Point2f> scene_fingertip_location) {
    
    Mat img_object = img_object_vector[curr_state_id];

    if (fingertip_location[0].x > 0 && fingertip_location[0].x < img_object.cols &&
        fingertip_location[0].y > 0 && fingertip_location[0].y < img_object.rows) {

        int num_ref_img = (int)img_object_vector.size() > 5 ? (int)img_object_vector.size() : 5;
        int ref_height = 750 / num_ref_img - 4;
        int ref_width = ref_height * 3 / 2;
        
        
        double ratio = img_object.rows > img_object.cols * 2.0 / 3.0 ? ref_height / (double)img_object.rows : ref_width / (double) img_object.cols;
        
        int xOffset = 225 - ref_width / 2;
        int yOffset = curr_state_id * (ref_height + 4) + 2;

        Point2f object_fingertip = fingertip_location[0] * ratio + Point2f(xOffset, yOffset);
        Point2f scene_fingertip = scene_fingertip_location[0] + Point2f( 450, 250);
        
        line( img_results, object_fingertip, scene_fingertip, Scalar(0, 255, 0), 4 );
        circle(img_results, object_fingertip, 5, Scalar(0,0,255), 4, 8, 0);
        circle(img_results, scene_fingertip, 5, Scalar(0,0,255), 4, 8, 0);
    }
}


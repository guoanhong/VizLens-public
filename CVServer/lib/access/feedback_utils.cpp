/* Feedback Utils Implementation */

#include "feedback_utils.hpp"
#include <set>

extern string id;
extern string image;
extern string mode;
extern string target;
extern string state;
extern string feedback;
extern string guidance;
extern int idle;

std::vector<std::string> &split(const std::string &s, char delim, std::vector<std::string> &elems) {
    std::stringstream ss(s);
    std::string item;
    while (std::getline(ss, item, delim)) {
        elems.push_back(item);
    }
    return elems;
}

std::vector<std::string> split(const std::string &s, char delim) {
    std::vector<std::string> elems;
    split(s, delim, elems);
    return elems;
}

string getFeedback(vector<vector<int> > button_map, vector<string> true_labels, vector<Point2f> fingertip_location, vector<Point2f> previous_fingertip_location, double avg_button_size) {
    int y = floor(fingertip_location[0].x);
    int x = floor(fingertip_location[0].y);
    string feedback = "";
    
    int magic_value = button_map[x][y];
    int size = true_labels.size();
    
    if (previous_fingertip_location[0].x == -1 && previous_fingertip_location[0].y == -1) {
        if (magic_value >= 10000) {
            feedback = "near " + true_labels[magic_value - 10000];
        } else if (magic_value > size) {
            feedback = "between " + true_labels[magic_value / 100] + " and " + true_labels[magic_value % 100];
        } else if (magic_value >= 0) {
            feedback = "at " + true_labels[magic_value] + ", press it";
        } else if (magic_value == -1) {
            feedback = "";
        }
    } else {
        int previous_y = floor(previous_fingertip_location[0].x);
        int previous_x = floor(previous_fingertip_location[0].y);
        int previous_magic_value = button_map[previous_x][previous_y];
        
        double distance = sqrt( (previous_fingertip_location[0].x - fingertip_location[0].x) *
                               (previous_fingertip_location[0].x - fingertip_location[0].x) +
                               (previous_fingertip_location[0].y - fingertip_location[0].y) *
                               (previous_fingertip_location[0].y - fingertip_location[0].y) );
        
        double thres = avg_button_size * 0.3; //10
        
        // Detect movement speed, if fast: say nothing; if slow: use words
        if (distance <= thres) {
            if (magic_value == previous_magic_value) {
                feedback = "";
            } else {
                if (magic_value >= 10000) {
                    feedback = "near " + true_labels[magic_value - 10000];
                } else if (magic_value > size) {
                    feedback = "between " + true_labels[magic_value / 100] + " and " + true_labels[magic_value % 100];
                } else if (magic_value >= 0) {
                    feedback = "at " + true_labels[magic_value] + ", press it";
                } else if (magic_value == -1) {
                    feedback = "";
                }
            }
        } else {
            if (magic_value >= 10000) {
                feedback = "";
            } else if (magic_value > size) {
                feedback = "";
            } else if (magic_value >= 0) {
                feedback = "";
            } else if (magic_value == -1) {
                feedback = "";
            }
        }
    }
    
    return feedback;
}

int getGuidanceIndex(vector<string> true_labels, string targets) {
    transform(targets.begin(), targets.end(), targets.begin(), ::tolower);

    vector<string> target = split(targets, ';');
    
    for (int i = 0; i < target.size(); i++) {
        for (int j = 0; j < true_labels.size(); j++) {
            if (!true_labels[j].compare(target[i])) {
                return j;
            }
        }
    }
    return -1;
}

int pathLength(vector<map<int, int>> transition_graph, int curr_id, int target_id) {

    if (curr_id == target_id) {
        return 0;
    }

    set<int> visited;
    vector<int> currLevel = vector<int>();
    vector<int> nextLevel = vector<int>();
    visited.insert(curr_id);
    currLevel.push_back(curr_id);
    int level = 0;
    while (visited.size() < transition_graph.size() && currLevel.size() > 0) {
        level++;
        for (int i = 0; i < currLevel.size(); i++) {
            int tmp_curr_id = currLevel[i];
            map<int, int> neighbors = transition_graph[tmp_curr_id];
            map<int, int>::iterator it;
            for (it = neighbors.begin(); it != neighbors.end(); it++) {
                int state = it->second;
                if (state == target_id) return level;
                if (visited.find(state) == visited.end()) {
                    visited.insert(state);
                    nextLevel.push_back(state);
                }
            }
        }
        currLevel = nextLevel;
        nextLevel = vector<int>();
    }
    return -1;
}

int getGuidanceIndexBasedOnExpectedPath(vector<int> state_path, vector<int> button_path, vector<map<int, int>> transition_graph, vector<vector<int>> button_id_vector, int curr_state_id, int &next_idx_on_path) {
    
    int guidance_button_id = -1;
    int target_state_id = state_path[next_idx_on_path];
    if (curr_state_id == target_state_id) {
        guidance_button_id = button_path[next_idx_on_path];
        next_idx_on_path++;
        if (next_idx_on_path >= state_path.size()) {
            cout << "No need to find guidance index since target is reached!" << endl;
            return -1;
        }
    } else {
        // Case 1: If the current state is not next target state
        // but it is somewhere on the path
        for (int i = 0; i < state_path.size(); i++) {
            if (curr_state_id == state_path[i]) {
                next_idx_on_path = i + 1;
                guidance_button_id = button_path[i];
            }
        }

        // Case 2: If the current state is not on the path
        if (guidance_button_id == -1) {
            map<int, int> neighbors = transition_graph[curr_state_id];
            int shortes_path = -1;
            map<int, int>::iterator it;
            for (it = neighbors.begin(); it != neighbors.end(); it++) {
                int curr_button_id = it->first;
                // Start over if something goes wrong
                // Search to find shortest path from next state of curr_state_id to initial state
                int path_length = pathLength(transition_graph, it->second, state_path[0]); 
                if (shortes_path == -1 || shortes_path > path_length) {
                    guidance_button_id = curr_button_id;
                    shortes_path = path_length;
                }
            }
            if (shortes_path < 0) {
                cout << "Did not find path from current state to next target state!" << endl;
                return -1;
            }
            next_idx_on_path = 0;
            cout << "Next button id to press: " << guidance_button_id << endl;
        }
    }

    // Find guidance button based on given button id
    vector<int> buttons = button_id_vector[curr_state_id];
    for (int i = 0; i < buttons.size(); i++) {
        if (guidance_button_id == buttons[i]) {
            return i;
        }
    }
    cout << "Target button not found on current screen!" << endl;
    return -1;
}

string getGuidance(vector<vector<int> > button_map, vector<string> true_labels, vector<double> guidance_button_center, vector<Point2f> fingertip_location, vector<Point2f> previous_fingertip_location, double avg_button_size) {
    string feedback = "";
    
    double guidance_distance = sqrt( (guidance_button_center[0] - fingertip_location[0].x) *
                                    (guidance_button_center[0] - fingertip_location[0].x) +
                                    (guidance_button_center[1] - fingertip_location[0].y) *
                                    (guidance_button_center[1] - fingertip_location[0].y) );
    
    double inner_threshold = avg_button_size * 0.5; //20
    double outer_threshold = avg_button_size * 1.5; //60
    
    if (guidance_distance <= inner_threshold) {
        feedback = getFeedback(button_map, true_labels, fingertip_location, previous_fingertip_location, avg_button_size);
    } else if (guidance_distance <= outer_threshold) {
        feedback = getDirection(guidance_button_center, fingertip_location, avg_button_size);
        feedback = feedback + " slowly";
    } else {
        feedback = getDirection(guidance_button_center, fingertip_location, avg_button_size);
    }
    
    return feedback;
}

string getGuidanceForPathMode(vector<vector<int> > button_map, vector<string> true_labels, vector<double> guidance_button_center, vector<double> guidance_button_shape, vector<Point2f> fingertip_location, vector<Point2f> previous_fingertip_location, double avg_button_size, int guidance_index) {
    
    string feedback = "";

    double buttonCenterX = guidance_button_center[0];
    double buttonCenterY = guidance_button_center[1];
    double buttonShapeX = guidance_button_shape[0];
    double buttonShapeY = guidance_button_shape[1];
    double fingerX = fingertip_location[0].x;
    double fingerY = fingertip_location[0].y; 

    // Note: This could potentially be changed to be more error-tolerant
    double inside_threshold = 0.9;
    double close_threshold = 1.5;
    bool finger_inside_button = (abs(fingerX - buttonCenterX) <= inside_threshold * buttonShapeX / 2)
                             && (abs(fingerY - buttonCenterY) <= inside_threshold * buttonShapeY / 2);
    bool finger_close_to_button = (abs(fingerX - buttonCenterX) <= close_threshold * buttonShapeX / 2)
                               && (abs(fingerY - buttonCenterY) <= close_threshold * buttonShapeY / 2);

    if (finger_inside_button) {
        feedback = "at " + true_labels[guidance_index] + ", press it";
    } else if (finger_close_to_button) {
        feedback = getDirectionForPathMode(guidance_button_center, guidance_button_shape, fingertip_location);
        feedback = feedback + " slowly";
    } else {
        feedback = getDirectionForPathMode(guidance_button_center, guidance_button_shape, fingertip_location);
    }
    
    return feedback;
}

string getDirection(vector<double> guidance_button_center, vector<Point2f> fingertip_location, double avg_button_size) {
    string feedback = "";
    
    double threshold = avg_button_size * 0.4; //15
    
    double x_diff = guidance_button_center[0] - fingertip_location[0].x;
    double y_diff = guidance_button_center[1] - fingertip_location[0].y;
    
    // abs(x_diff) for always longest distance
    if (abs(y_diff) > threshold) {
        feedback = y_diff < 0 ? "move up" : "move down";
    } else {
        feedback = x_diff < 0 ? "move left" : "move right";
    }
    
    return feedback;
}

string getDirectionForPathMode(vector<double> guidance_button_center, vector<double> guidance_button_shape, vector<Point2f> fingertip_location) {
    
    string feedback = "";
    
    double buttonCenterX = guidance_button_center[0];
    double buttonCenterY = guidance_button_center[1];
    double buttonShapeX = guidance_button_shape[0];
    double buttonShapeY = guidance_button_shape[1];
    double fingerX = fingertip_location[0].x;
    double fingerY = fingertip_location[0].y; 

    if (fingerX < buttonCenterX - buttonShapeX / 2) {
        feedback = "move right";
    } else if (fingerX > buttonCenterX + buttonShapeX / 2) {
        feedback = "move left";
    } else if (fingerY < buttonCenterY - buttonShapeY / 2) {
        feedback = "move down";
    } else {
        feedback = "move up";
    }
    
    return feedback;
}

void anounceFeedback(string feedback) {
    // Anounce feedback string
    stringstream ss;
    ss << "say \"" << feedback << "\"";
    system(ss.str().c_str());
}

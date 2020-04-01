/* State Map Implementation */

#include "statemap_utils.hpp"
#include "db_utils.hpp"

extern string id;
extern string image;
extern string mode;
extern string target;
extern string state;
extern string feedback;
extern string guidance;
extern int idle;

int findCurrentState(vector<int> states) {
    int majority;
    majority = states.back();
    int count = 0;
    for (int i = 0; i < states.size(); ++i) {
        if (count == 0) {
            majority = states[i];
            count = 1;
        } else if (states[i] == majority) {
            ++count;
        } else {
            --count;
        }
    }
    if (count == 0) {
        return states.back();
    }
    return majority;
}

Point2f findTransitionFingertipLocation(int new_id, vector<int> states, vector<Point2f> fingertip_locations) {
    for (int i = 0; i < states.size(); i++) {
        if (states[i] == new_id) 
            return fingertip_locations[i];
    }
    return Point2f(0.f, 0.f);
}

void getCandidateObjectIds(int curr_state_id, vector<int> all_ids, vector<map<int, int>> transition_graph, vector<int> &candidate_ids_object_vector) {
    /* At the beginning, all reference images are candidates */
    if (curr_state_id < 0) {
        candidate_ids_object_vector = all_ids;
        return;
    }
    /* Add all neighbors of the previous state to candidates */
    map<int, int> neighbors = transition_graph[curr_state_id];
    map<int, int>::iterator it;
    set<int> neighbor_ids;
    for (it = neighbors.begin(); it != neighbors.end(); it++) {
        int target_state = it->second; // Access value (target state id)
        // This id is not added to candidates yet
        if (neighbor_ids.find(target_state) == neighbor_ids.end()) {
            neighbor_ids.insert(target_state);
            candidate_ids_object_vector.push_back(target_state);
        }
    }
    /* Add itself to candidates as well */
    candidate_ids_object_vector.push_back(curr_state_id);
}

void getCandidateObjectDescriptors(vector<int> candidate_ids_object_vector, vector<Mat> descriptors_object_gpu_vector, vector<Mat> &candidate_descriptors_object_vector) {
    /* At the beginning, all reference images are candidates */
    /* Might not work
    if (candidate_ids_object_vector.size() == descriptors_object_gpu_vector.size()) {
        candidate_descriptors_object_vector = descriptors_object_gpu_vector;
        return;
    }
    */

    /* Add candidate descriptors according to ids */
    for (int i = 0; i < candidate_ids_object_vector.size(); i++) {
        candidate_descriptors_object_vector.push_back(descriptors_object_gpu_vector[candidate_ids_object_vector[i]]);
    }
}

void addStateToTransitionGraph(string session_name, vector<map<int, int>> &transition_graph, int new_state_id) {
    // TODO(judy): Rewrite add transition graph using map<int, int>
    if (transition_graph.size() <= new_state_id) {
        map<int, int> new_neighbors = map<int, int>();
        transition_graph.push_back(new_neighbors);
    }
    cout << "Added new state to transition graph" << endl;
}

void updateTransitionGraph(string session_name, vector<map<int, int>> &transition_graph, int curr_state_id, int new_state_id, int button_id) {
    // Add connection from curr state to new state if this is not the only state
    if (curr_state_id >= 0) {
        map<int,int>::iterator it = transition_graph[curr_state_id].begin();
        transition_graph[curr_state_id].insert(it, pair<int,int>(button_id, new_state_id));
        // Might need to update database for new transitions
        addTransitionToDB(session_name, curr_state_id, new_state_id, button_id);
    }
    // If find a new state, add it the the transition graph
    if (transition_graph.size() <= new_state_id) {
        map<int, int> new_state_map;
        transition_graph.push_back(new_state_map);
    }
}

void initializeObjects(vector<set<int>> adjList, vector<set<int>> &reachable_states) {
    // Allow states within 2 steps
    reachable_states = adjList;
    for (int i = 0; i < adjList.size(); ++i) {
        set<int> neighbors = reachable_states[i];
        for (const int &neighbor : neighbors) {
            reachable_states[i].insert(reachable_states[neighbor].begin(), reachable_states[neighbor].end());
        }
    }
}

void filterObjects(int prev_state_id, vector<set<int>> reachable_states, vector<Mat> &reachable_descriptors_object_gpu_vector, vector<Mat> descriptors_object_gpu_vector) {
    set<int> reachables = reachable_states[prev_state_id];
    set<int>::iterator it;
    for (it = reachables.begin(); it != reachables.end(); it++) {
        reachable_descriptors_object_gpu_vector.push_back(descriptors_object_gpu_vector[*it]);
    }
}

void filterOriginalImgs(int prev_state_id, vector<set<int>> reachable_states, vector<Mat> &reachable_img_object_vector, vector<Mat> img_object_vector) {
    set<int> reachables = reachable_states[prev_state_id];
    set<int>::iterator it;
    for (it = reachables.begin(); it != reachables.end(); it++) {
        reachable_img_object_vector.push_back(img_object_vector[*it]);
    }
}

/*
string storeTransitionGraph(vector<map<int, int>> transition_graph) {
    string output;
    for (int i = 0; i < transition_graph.size(); i++) {
        // vector<int> curr_set(transition_graph.size());
        // copy(transition_graph[i].begin(), transition_graph[i].end(), curr_set.begin());
        ostringstream stream;
        copy(transition_graph[i].begin(), transition_graph[i].end(), ostream_iterator<int>(stream, " "));
        string curr_set = stream.str().c_str();
        output += to_string(transition_graph[i].size()) + " " + curr_set + "\n";
    }
    return output;
}
*/

int getExpectedNextState(vector<map<int, int>> transition_graph, int curr_state_id, int button_index) {
    map<int, int> neighbors = transition_graph[curr_state_id];
    if (neighbors.find(button_index) == neighbors.end()) {
        cout << "NOT FOUND BUTTON INDEX IN NEIGHBORS" << endl;
        return -1;
    }
    cout << "FOUND " << button_index << endl; 
    return neighbors[button_index];
}

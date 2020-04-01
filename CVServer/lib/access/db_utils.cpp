/* Database Utils Implementation */
#include "db_utils.hpp"
#include <fstream>

extern bool isOpenDB;
extern sqlite3 *dbfile;
extern string id;
extern string image;
extern string mode;
extern string target;
extern string state;
extern string feedback;
extern string guidance;
extern int idle;
extern int curr_state_id;

using namespace std;

bool ConnectDB(const char* DB) {
    if ( sqlite3_open(DB, &dbfile) == SQLITE_OK ) {
        isOpenDB = true;
        return true;
    }
    return false;
}

void DisonnectDB(const char* DB) {
    if ( isOpenDB == true ) {
        sqlite3_close(dbfile);
    }
}

int getdir (string dir, vector<string> &files) {
    DIR *dp;
    struct dirent *dirp;
    if((dp  = opendir(dir.c_str())) == NULL) {
        cout << "Error(" << errno << ") opening " << dir << endl;
        return errno;
    }
    
    while ((dirp = readdir(dp)) != NULL) {
        files.push_back(string(dirp->d_name));
    }
    closedir(dp);
    return 0;
}


void getExpectedPath(vector<int> &state_path, vector<int> &button_path) {
    while (state_path.size() == 0) {
        ifstream inFilePath;
        inFilePath.open("../AgentWebhook/instructions.txt");
        string line;
        // Read state path
        getline(inFilePath, line);
        stringstream states(line);
        cout << line << endl;
        while(states.good())
        {
            string substr;
            getline(states, substr, ',');
            state_path.push_back(stoi(substr));
        }
        // Read button path
        getline(inFilePath, line);
        stringstream buttons(line);
        while(buttons.good())
        {
            string substr;
            getline(buttons, substr, ',');
            button_path.push_back(stoi(substr));
        }
        inFilePath.close();
        // state_path = {0, 1, 2, 3, 4, 13};
        // button_path = {1, 6, 10, 17, 20};
    }
    cout << "State path: ";
    for (int i = 0; i < state_path.size(); i++) {
        cout << state_path[i] << ", ";
    }
    cout << endl << "Button path: ";
    for (int i = 0; i < button_path.size(); i++) {
        cout << button_path[i] << ", ";
    }
    cout << endl;
}

void getRefImageFilenames(string session_name, string dir, vector<string> &filenames, string mode) {

    if (mode == "local") {
        /* Read reference images from given local directory in "local" mode */
        getdir(dir, filenames);
    } else {
        /* Read from given session in database in "db" mode */
        sqlite3_stmt *statement;    
        char query[1000];
        sprintf(query, "SELECT image FROM objects WHERE session=\'%s\' AND (status=\'ready\')", session_name.c_str());
        cout << query << endl;
        
        if ( sqlite3_prepare(dbfile, query, -1, &statement, 0 ) == SQLITE_OK ) {
            while (true) {
                int nextResult = sqlite3_step(statement);
                if (nextResult == SQLITE_DONE)
                   break;
                if (nextResult != SQLITE_ROW) {
                   printf("error: %s!\n", sqlite3_errmsg(dbfile));
                   break;
                }
                string curr_image;
                cout << "Got it." << endl;  
                curr_image = (char*)sqlite3_column_text(statement, 0);
                filenames.push_back(curr_image);
                cout << curr_image << endl;
                // idle = 0;
            }
            sqlite3_finalize(statement);
        }
    }

    /* Make sure filenames are sorted */
    sort(filenames.begin(), filenames.end());
    cout << "Read Filenames From Database: " << filenames.size() << endl;
    for (int i = 0; i < filenames.size(); i++) {
        cout << filenames[i] << endl;
    }
}

void getTransitionGraph(string session_name, string read_mode, vector<string> filenames, vector<map<int, int>> &transition_graph) {

    /* Initialize empty transition graph for given number of states */
    for (int i = 0; i < filenames.size(); i++) {
        map<int, int> nbrs;
        transition_graph.push_back(nbrs);
    }

    if (mode == "local") {
        // TODO: read local transition graph
        // (Right now start from empty)
    } else {
        /* Read from object_transitions table from db in "db" mode */
        sqlite3_stmt *statement;    
        char query[1000];
        sprintf(query, "SELECT stateid, buttonid, targetstateid FROM object_transitions WHERE session=\'%s\'", session_name.c_str());
        cout << query << endl;
        
        if ( sqlite3_prepare(dbfile, query, -1, &statement, 0 ) == SQLITE_OK ) {
            while (true) {
                int nextResult = sqlite3_step(statement);
                if (nextResult == SQLITE_DONE)
                   break;
                if (nextResult != SQLITE_ROW) {
                   printf("error: %s!\n", sqlite3_errmsg(dbfile));
                   break;
                }
                int state_id = atoi((char*)sqlite3_column_text(statement, 0));
                int button_id = atoi((char*)sqlite3_column_text(statement, 1));
                int target_state_id = atoi((char*)sqlite3_column_text(statement, 2));
                transition_graph[state_id][button_id] = target_state_id;
                cout << "state_id=" << state_id << "; button_id=" << button_id << "; target_state_id=" << target_state_id << endl;
                idle = 0;
            }
            sqlite3_finalize(statement);
        }
    }

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

    /* Manually constructed example statemap */
    /* 
    if (transition_graph.size() == 0) {
        int init0[] = {0};
        set<int> nbrs0(init0, init0 + 1);
        transition_graph.push_back(nbrs0);
    }

    for (int i = 0; i < 11; i++) {
        int init[] = {i, i + 1};
        set<int> nbrs(init, init + 2);
        transition_graph.push_back(nbrs);
    }
    int init[] = {11};
    set<int> nbrs(init, init + 1);
    transition_graph.push_back(nbrs);

    int init0[] = {0, 1, 4};
    set<int> nbrs0(init0, init0 + 3);
    transition_graph.push_back(nbrs0);
    int init1[] = {0, 1, 2, 3};
    set<int> nbrs1(init1, init1 + 4);
    transition_graph.push_back(nbrs1);
    int init2[] = {1, 2};
    set<int> nbrs2(init2, init2 + 2);
    transition_graph.push_back(nbrs2);
    int init3[] = {1, 3};
    set<int> nbrs3(init3, init3 + 2);
    transition_graph.push_back(nbrs3);
    int init4[] = {0, 4};
    set<int> nbrs4(init4, init4 + 2);
    transition_graph.push_back(nbrs4);
    */
}

void getImageToProcess(string &image_to_process) {
    sqlite3_stmt *statement;
    
    char query[1000];
    
    sprintf(query, "SELECT image, id, imageactive, target FROM images_video WHERE imageactive>0 AND id>%s ORDER BY id DESC", id.c_str());

    // cout << query << endl;
    
    if ( sqlite3_prepare(dbfile, query, -1, &statement, 0 ) == SQLITE_OK ) {
        int firstResult = sqlite3_step(statement);
        if (firstResult == 100) {
            cout << "Got it." << endl;
            image_to_process = (char*)sqlite3_column_text(statement, 0);
            cout << "image to process: " << image_to_process << endl;
            id = (char*)sqlite3_column_text(statement, 1);
            cout << "id: " << id << endl;
            // mode = (char*)sqlite3_column_text(statement, 2);
            // cout << "mode: " << mode << endl;
            target = (char*)sqlite3_column_text(statement, 3);
            cout << "target: " << target << endl;
            idle = 0;
        } else if (firstResult == 101) {
            // cout << "Nothing." << endl;
        }
        sqlite3_finalize(statement);
    }
}

int updateImage(int objectfound, int fingerfound, int fingerx, int fingery) {
    sqlite3_stmt *statement;
    int result;
    
    char query[1000];
    
    sprintf(query, "UPDATE images_video SET stateid=%d, imageactive=0,"
            "feedback=\"%s\",feedbackactive=1,resulttime=current_timestamp, objectfound=%s, fingerfound=%s, fingertipx=%s, fingertipy=%s WHERE id=%s", curr_state_id,
            feedback.c_str(), to_string(objectfound).c_str(), to_string(fingerfound).c_str(), to_string(fingerx).c_str(), to_string(fingery).c_str(), id.c_str());
    
    cout << query << endl;
    
    {
        if(sqlite3_prepare(dbfile,query,-1,&statement,0)==SQLITE_OK) {
            int res = sqlite3_step(statement);
            result = res;
            sqlite3_finalize(statement);
            
            cout << "Update Image!" << endl;
        }
        return result;
    }
    
    cout << "Update Image!" << endl;
    
    return 0;
}

int skipImage() {
    sqlite3_stmt *statement;
    int result;
    
    char query[1000];
    
    sprintf(query, "UPDATE images_video SET imageactive=0 WHERE id=%s", id.c_str());
    
    cout << query << endl;
    
    {
        if(sqlite3_prepare(dbfile,query,-1,&statement,0)==SQLITE_OK) {
            int res = sqlite3_step(statement);
            result = res;
            sqlite3_finalize(statement);
            
            cout << "Skip Image!" << endl;
        }
        return result;
    }
    
    cout << "Skip Image!" << endl;
    
    return 0;
}

void sendTransitionInstructions(string session, string instruction) {
    sqlite3_stmt *statement;
    int result;
    char query[1000];
    sprintf(query, "UPDATE images_video SET feedback=\'%s\', feedbackactive=1, imageactive=0 WHERE session=\'%s\' AND id=%s", instruction.c_str(), session.c_str(), id.c_str());
    cout << query << endl;
    {
        if(sqlite3_prepare(dbfile,query,-1,&statement,0)==SQLITE_OK) {
            int res = sqlite3_step(statement);
            result = res;
            sqlite3_finalize(statement);
            
            cout << "Sent waiting/resuming instructions!" << endl;
        }
    }
}

string getStateDescription(string session, int state_id) {

    // TODO: PULL STATE DESCRIPTION FROM DB ACCORDING TO STATE ID & ADD TO IMAGES_VIDEO DATABASE
    sqlite3_stmt *statement;
    char query1[1000];
    sprintf(query1, "SELECT description FROM objects WHERE session=\'%s\' AND stateid=%d AND (status=\'ready\' OR status=\'readytoread\')", session.c_str(), state_id);
    cout << query1 << endl;
    
    string description_text;
    if ( sqlite3_prepare(dbfile, query1, -1, &statement, 0 ) == SQLITE_OK ) {
        int firstResult = sqlite3_step(statement);
        if (firstResult == 100) {
            cout << "Got it." << endl;
            description_text = (char*)sqlite3_column_text(statement, 0);
            cout << description_text << endl;
            sqlite3_finalize(statement);
            return description_text;
        } else if (firstResult == 101) {
            cout << "Nothing." << endl;
        }
        sqlite3_finalize(statement);
    }
    return "no description";

    // int result;
    // char query2[1000];
    // sprintf(query2, "UPDATE images_video SET stateid=%d, feedback=\'%s\', feedbackactive=1, imageactive=0 WHERE session=\'%s\' AND id=%s", state_id, description_text.c_str(), session.c_str(), id.c_str());
    // cout << query2 << endl;
    // {
    //     if(sqlite3_prepare(dbfile,query2,-1,&statement,0)==SQLITE_OK) {
    //         int res = sqlite3_step(statement);
    //         result = res;
    //         sqlite3_finalize(statement);
            
    //         cout << "Updated description!" << endl;
    //     }
    //     // return result;
    // }
}

bool hasReadyToReadObjects() {
    sqlite3_stmt *statement;
    char query[1000];
    sprintf(query, "SELECT image FROM objects WHERE status=\'readytoread\'");
    cout << query << endl;
    
    if ( sqlite3_prepare(dbfile, query, -1, &statement, 0 ) == SQLITE_OK ) {
        int firstResult = sqlite3_step(statement);
        if (firstResult == 100) {
            cout << "Got it." << endl;
            return true;
            // idle = 0;
        } else if (firstResult == 101) {
        }
        sqlite3_finalize(statement);
    }
    cout << "No new image to read!" << endl;
    return false;
}

void updateCroppedImage(string session, string dir_server, string &new_name, int &state_id, Mat &img_object) {

    sqlite3_stmt *statement;
    char query1[1000];
    sprintf(query1, "SELECT image, stateid FROM objects WHERE session=\'%s\' AND status=\'readytoread\'", session.c_str());
    cout << query1 << endl;
    
    while (true) {
        if ( sqlite3_prepare(dbfile, query1, -1, &statement, 0 ) == SQLITE_OK ) {
            int firstResult = sqlite3_step(statement);
            if (firstResult == 100) {
                cout << "Got it." << endl;
                new_name = (char*)sqlite3_column_text(statement, 0);
                cout << new_name << endl;
                state_id = atoi((char*)sqlite3_column_text(statement, 0));
                img_object = imread(dir_server + new_name);
                // idle = 0;
                break;
            } else if (firstResult == 101) {
                // cout << "Nothing." << endl;
            }
            sqlite3_finalize(statement);
        }
    }

    int result;
    char query2[1000];
    sprintf(query2, "UPDATE objects SET status=\'ready\' WHERE session=\'%s\' AND status=\'readytoread\'", session.c_str());
    cout << query2 << endl;
    {
        if(sqlite3_prepare(dbfile,query2,-1,&statement,0)==SQLITE_OK) {
            int res = sqlite3_step(statement);
            result = res;
            sqlite3_finalize(statement);
            cout << "Updated to ready!" << endl;
        }
    }    

}


void labels_hash(string session_name, int state_index, Mat img_object, double ratio, vector<int> &button_id, vector<vector<int> > &button_map, vector<string> &true_labels, vector<vector<double> > &button_center, vector<vector<double> > &button_shape, double &avg_button_size) {
    string id, x1, y1, x2, y2, label;
    vector<vector<string> > labels;
    sqlite3_stmt *statement;
    
   // state_index = 0;

   char query[1000];
   sprintf(query, "SELECT id, x1, y1, x2, y2, label FROM object_buttons WHERE session=\"%s\" AND stateid=%d", session_name.c_str(), state_index);
   cout << query << endl;
    
   if ( sqlite3_prepare(dbfile, query, -1, &statement, 0 ) == SQLITE_OK ) {
    // string x1s[] = {"12","449","886","13","454","889","3","440","870","167","705","84","507","927","86","505","928","82","507","922","83","502","917","79","724"};
    // string y1s[] = {"378","384","387","794","800","803","1195","1200","1204","1551","1552","1968","1974","1978","2314","2317","2320","2659","2653","2658","2993","2990","2991","3389","3384"};
    // string x2s[] = {"411","844","1280","415","844","1280","425","858","1282","582","1124","363","781","1202","356","791","1203","373","777","1208","368","785","1194","565","1196"};
    // string y2s[] = {"614","614","624","1028","1033","1030","1430","1434","1436","1779","1780","2221","2225","2227","2565","2566","2567","2902","2902","2904","3250","3243","3234","3639","3638"};
    // string label_input[] = {"baked potato","popcorn","pizza","beverage","frozen dinner","reheat","power level","time cook","time defrost","kitchen timer","weight defrost","1","2","3","4","5","6","7","8","9","clock","0","add 30 sec","stop clear","start pause"};
    
    while (true) {
            
           int nextResult = sqlite3_step(statement);
           
           if (nextResult == SQLITE_DONE)
               break;
           
           if (nextResult != SQLITE_ROW) {
//                printf("error: %s!\n", sqlite3_errmsg(dbfile));
               break;
           }
            
           cout << "Got it." << endl;

            id = (char*)sqlite3_column_text(statement, 0);
            cout << id << endl;
            // x1 = x1s[i]; 
            x1 = (char*)sqlite3_column_text(statement, 1);
            cout << x1 << endl;
            // y1 = y1s[i]; 
            y1 = (char*)sqlite3_column_text(statement, 2);
            cout << y1 << endl;
            // x2 = x2s[i]; 
            x2 = (char*)sqlite3_column_text(statement, 3);
            cout << x2 << endl;
            // y2 = y2s[i];
            y2 = (char*)sqlite3_column_text(statement, 4);
            cout << y2 << endl;
            // label = label_input[i]; 
            label = (char*)sqlite3_column_text(statement, 5);
            cout << label << endl;
            
            // push x1, y1, x2, y2, label into x and push to labels
            vector<string> x;
            x.push_back(x1);
            x.push_back(y1);
            x.push_back(x2);
            x.push_back(y2);
            x.push_back(label);
            labels.push_back(x);

            // Add button id to button ids
            button_id.push_back(stoi(id));
            cout << "Got it." << endl;
        }
       
       sqlite3_finalize(statement);
   } else {
       cout << "Failed" << endl;
   }
    
    double sum_button_width = 0;
    
    // compute the centers of each button
    for (int i = 0; i < labels.size(); i++) {
        vector<string> tmp = labels[i];
        double x1 = stod(tmp[0]) * ratio;
        double y1 = stod(tmp[1]) * ratio;
        double x2 = stod(tmp[2]) * ratio;
        double y2 = stod(tmp[3]) * ratio;
        double centerX = (x1 + x2) / 2;
        double centerY = (y1 + y2) / 2;
        double shapeX = abs(x2 - x1);
        double shapeY = abs(y2 - y1);
        vector<double> append_center = {centerX, centerY};
        vector<double> append_shape = {shapeX, shapeY};
        button_center.push_back(append_center);
        button_shape.push_back(append_shape);
        true_labels.push_back(tmp[4]);  // push button labels
        
        sum_button_width += x2 - x1;
    }
    
    avg_button_size = sum_button_width / labels.size();
    
    double threshold = avg_button_size; //55
    cout << avg_button_size << endl;
    // Generate two dimensional array, assign class label
    cout << img_object.rows << " " << img_object.cols << endl;
    for (int i = 0; i < img_object.rows; i++) {
        vector<int> row;
        for (int j = 0; j < img_object.cols; j++) {
            int distance[labels.size()];
            int idx_min1 = -1;     double min_d1 = 100000;
            int idx_min2 = -1;     double min_d2 = 100000;
            for (int k = 0; k < labels.size(); k++) {
                double centerX = button_center[k][0];
                double centerY = button_center[k][1];
                distance[k] = sqrt((centerX - j) * (centerX - j) + (centerY - i) * (centerY - i));
                if (distance[k] < min_d2) {
                    if (distance[k] < min_d1) {
                        min_d2 = min_d1;
                        min_d1 = distance[k];
                        idx_min2 = idx_min1;
                        idx_min1 = k;
                    } else {
                        min_d2 = distance[k];
                        idx_min2 = k;
                    }
                }
            }
            
            // check whether in the smallest distance
            vector<string> tmp = labels[idx_min1];
            double x1 = stod(tmp[0]) * ratio;
            double y1 = stod(tmp[1]) * ratio;
            double x2 = stod(tmp[2]) * ratio;
            double y2 = stod(tmp[3]) * ratio;
            
            if (x1 <= j && j <= x2 && i >= y1 && i <= y2) {
                row.push_back(idx_min1);
            } else if (min_d1 > threshold) {
                row.push_back(-1);
            } else if (min_d2 > threshold) {
                row.push_back(idx_min1 + 10000);
            } else {
                row.push_back(idx_min1 * 100 + idx_min2);
            }
        }
        button_map.push_back(row);
    }
    
}

void addTransitionToDB(string session_name, int curr_state_id, int new_state_id, int button_id) {
    sqlite3_stmt *statement;
    int result;
    char query[1000];
    sprintf(query, "INSERT INTO object_transitions (session, stateid, buttonid, targetstateid, seqinfo) VALUES (\'%s\', %d, %d, %d, \'\')", session_name.c_str(), curr_state_id, button_id, new_state_id);
    cout << query << endl;
    {
        if (sqlite3_prepare(dbfile,query,-1,&statement,0)==SQLITE_OK) {
            int res = sqlite3_step(statement);
            result = res;
            sqlite3_finalize(statement);
            cout << "Inserted transition to table!" << endl;
        }
    }       
}


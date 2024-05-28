#include "logger.hpp"
#include "csvhandler.hpp"
#include "consts.hpp"
#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <sys/wait.h>
#include <fcntl.h>
#include <map>
#include <algorithm>

using namespace std;

static Logger logger("voter.cpp");

vector<int> str_to_vec(string str) {
    vector<int> vec;
    istringstream iss(str);
    string word;
    while (getline(iss, word, ' ')) {
        vec.push_back(stoi(word));
    }
    return vec;
}

int open_fifo_file(int o_flag) {
    int fd = open(FIFO_FILE, o_flag);
    if (fd == -1){
        logger.log_error("Failed to open fifo file");
        exit(EXIT_FAILURE);
    }
    return fd;
}

int main() {
    char validation_path[512] = {0};
    int fifo_fd = open_fifo_file(O_RDONLY);
    read(fifo_fd, validation_path, sizeof(validation_path));
    close(fifo_fd);
    string labels_path = string(validation_path) + "/labels.csv";
    CSV_Handler labels_file(labels_path, logger);
    vector<string> labels = labels_file.get_column("Class Number");
    vector<vector<int>> predictions;
    while (true) {
        string line;
        getline(cin, line);
        vector<int> prediction = str_to_vec(line);
        predictions.push_back(prediction);
        fifo_fd = open_fifo_file(O_WRONLY);
        string next = "next";
        write(fifo_fd, next.c_str(), next.size());
        close(fifo_fd);
        char done[32] = {0};
        fifo_fd = open_fifo_file(O_RDONLY);
        read(fifo_fd, done, sizeof(done));
        close(fifo_fd);
        if (strcmp(done, "done") == 0)
            break;
    }
    vector<int> final_prediction;
    for (int i = 0; i < predictions[0].size(); ++i) {
        map<int, int> count_map;
        for (const auto& prediction : predictions) {
            ++count_map[prediction[i]];
        }
        auto max_ele = max_element(count_map.begin(), count_map.end(),
                                  [](const auto& a, const auto& b) {
                                    if (a.second == b.second) {
                                        return a.first > b.first;
                                    }
                                    return a.second < b.second;
                                  });
        final_prediction.push_back(max_ele->first);
    }
    pair<int, int> result = make_pair(0, 0);
    for (int i = 0; i < labels.size(); ++i) {
        if (stoi(labels[i]) == final_prediction[i]) ++result.first;
        else ++result.second;
    }
    string result_str = to_string(result.first) + " " + to_string(result.second) + " ";
    fifo_fd = open_fifo_file(O_WRONLY);
    write(fifo_fd, result_str.c_str(), result_str.size());
    close(fifo_fd);
    exit(EXIT_SUCCESS);
}
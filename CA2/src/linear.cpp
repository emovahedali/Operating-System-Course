#include "logger.hpp"
#include "csvhandler.hpp"
#include "data.hpp"
#include "consts.hpp"
#include <iostream>
#include <string>
#include <vector>
#include <sys/wait.h>

using namespace std;

static Logger logger("linear.cpp");

string vec_to_str(vector<int> vec) {
    string str;
    for (int i = 0; i < vec.size(); ++i) {
        if (i == 0)
            str = to_string(vec[i]) + " ";
        else
            str += to_string(vec[i]) + " ";
    }
    return str;
}

vector<int> predict(CSV_Handler& dataset, CSV_Handler& classifier) {
    vector<int> prediction;
    for (int indx = 0; indx < dataset.get_size(); ++indx) {
        Data data(dataset.get_cell("Length", indx),
                  dataset.get_cell("Width", indx));
        prediction.push_back(data.predict(classifier));
    }
    return prediction;
}

int main() {
    string validation_path, weights_path;
    cin >> validation_path >> weights_path;
    string dataset_path = validation_path + "/dataset.csv";
    CSV_Handler dataset(dataset_path, logger);
    int received_classifiers = 0;
    while (received_classifiers < NUM_OF_CLASSIFIERS) {
        string indx;
        cin >> indx;
        string classifier_path = weights_path + "/classifier_" + indx + ".csv";
        CSV_Handler classifier(classifier_path, logger);
        vector<int> prediction = predict(dataset, classifier);
        cout << vec_to_str(prediction) << "\n";
        ++received_classifiers;
    }
    exit(EXIT_SUCCESS);
}
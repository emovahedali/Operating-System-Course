#ifndef DATA_HPP
#define DATA_HPP

#include <string>
#include <vector>
#include "csvhandler.hpp"

using namespace std;

class Data {
public:
    Data(string length, string width);
    int predict(CSV_Handler& classifier);
private:
    double dot_product(double betha_0, double betha_1, double Bias);
    int find_index_of_max_score(vector<double> scores);
    double length;
    double width;
};

#endif
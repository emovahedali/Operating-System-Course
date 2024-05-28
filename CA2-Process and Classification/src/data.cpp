#include "data.hpp"

Data::Data(string length, string width) {
    this->length = stod(length);
    this->width = stod(width);
}

int Data::predict(CSV_Handler& classifier) {
    vector<double> scores;
    for (int indx = 0; indx < classifier.get_size(); ++indx) {
        double score = dot_product(stod(classifier.get_cell("Betha_0", indx)), 
                                    stod(classifier.get_cell("Betha_1", indx)), 
                                    stod(classifier.get_cell("Bias", indx)));
        scores.push_back(score);
    }
    return find_index_of_max_score(scores);
}

double Data::dot_product(double betha_0, double betha_1, double Bias) {
    return betha_0 * length + betha_1 * width + Bias;
}

int Data::find_index_of_max_score(vector<double> scores) {
    pair<double, int> max_score = make_pair(scores[0], 0);
    for (int i = 0; i < scores.size(); ++i) {
        if (scores[i] > max_score.first) {
            max_score.first = scores[i];
            max_score.second = i;
        }
    }
    return max_score.second;
}
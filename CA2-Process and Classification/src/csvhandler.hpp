#ifndef CSV_HANDLER_HPP
#define CSV_HANDLER_HPP

#include <cstring>
#include <fstream>
#include <vector>
#include "logger.hpp"

using namespace std;
using Column = pair<string, vector<string>>;

class CSV_Handler {
public:
    CSV_Handler(const string path, Logger &logger);
    vector<string> get_column(string key);
    string get_cell(string key, int index);
    int get_size();
private:
    vector<Column> file;
    ifstream ifs;
    int size;
    void set_columns();
    void read_rows();
    void read_row(string row);
};

#endif
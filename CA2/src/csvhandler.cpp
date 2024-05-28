#include "csvhandler.hpp"
#include <iostream>

#include <sstream>

CSV_Handler::CSV_Handler(const string path, Logger &logger) {
    ifs.open(path);
    if (!ifs.is_open()) {
        logger.log_error("Failed to open the CSV file");
        exit(EXIT_FAILURE);
    }
    this->size = 0;
    set_columns();
    read_rows();
    ifs.close();
}

void CSV_Handler::set_columns() {
    string line;
    getline(ifs, line);
    istringstream iss(line);
    string title;
    while (getline(iss, title, ',')) {
        file.push_back({title, vector<string>()});
    }
}

void CSV_Handler::read_rows() {
    string line;
    while (getline(ifs, line)) {
        read_row(line);
        ++size;
    }
}

void CSV_Handler::read_row(string row) {
    istringstream iss(row);
    string cell;
    for (auto &column : file) {
        getline(iss, cell, ',');
        column.second.push_back(cell);
    }
}

vector<string> CSV_Handler::get_column(string key) {
    for (auto &column : file)
        if (column.first == key)
            return column.second;
    return vector<string>();
}

string CSV_Handler::get_cell(string key, int index) {
    return get_column(key)[index];
}

int CSV_Handler::get_size() { return size; }
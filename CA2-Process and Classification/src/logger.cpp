#include "logger.hpp"
#include "color.hpp"
#include <sys/wait.h>
#include <iostream>
#include <cstring>
#include <sstream>

#define MAX_SIZE_BUFF 1024

using namespace std;

Logger::Logger(string logger_name) {
    this->logger_name = logger_name;
}

string Logger::make_msg(const string& fmt, va_list args) {
    char msg[MAX_SIZE_BUFF];
    vsnprintf(msg, sizeof(msg), fmt.c_str(), args);
    return msg;
}

void Logger::log_info(const string& fmt, ...) {
    ostringstream oss;
    oss << Color::GRN << "[INFO] " << this->logger_name << ": "; 
    va_list args;
    va_start(args, fmt);
    oss << make_msg(fmt, args);
    va_end(args);
    oss << Color::RST;
    cerr << oss.str() << endl;
}

void Logger::log_error(const string& fmt, ...) {
    ostringstream oss;
    oss << Color::RED << "[ERROR] " << this->logger_name << ": "; 
    va_list args;
    va_start(args, fmt);
    oss << make_msg(fmt, args);
    va_end(args);
    oss << Color::RST;
    cerr << oss.str() << endl;
}

void Logger::log_error() {
    string error = strerror(errno);
    log_error(error);
}
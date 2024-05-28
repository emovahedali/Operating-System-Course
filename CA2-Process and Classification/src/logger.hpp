#ifndef LOGGER_HPP
#define LOGGER_HPP

#include <string>
#include <cstdarg>

using namespace std;

class Logger {
public:
    Logger(string logger_name);
    void log_info(const string& fmt, ...);
    void log_error(const string& fmt, ...);
    void log_error();
private:
    string make_msg(const string& fmt, va_list args);
    string logger_name;
};

#endif
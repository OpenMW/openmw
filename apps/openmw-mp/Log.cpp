//
// Created by koncord on 15.08.16.
//

#include <cstdarg>
#include <iostream>
#include <cstring>
#include "Log.hpp"

using namespace std;

Log *Log::sLog = nullptr;

Log::Log(int logLevel) : logLevel(logLevel)
{

}

void Log::Create(int logLevel)
{
    if(sLog != nullptr)
        return;
    sLog = new Log(logLevel);
}

void Log::Delete()
{
    if(sLog == nullptr)
        return
    delete sLog;
    sLog = nullptr;
}

const Log &Log::Get()
{
    return *sLog;
}

const char* getTime()
{
    time_t t = time(0);
    struct tm *tm = localtime(&t);
    static char result[20];
    sprintf(result, "%.4d-%.2d-%.2d %.2d:%.2d:%.2d",
            1900 + tm->tm_year, tm->tm_mon, tm->tm_mday,
            tm->tm_hour, tm->tm_min, tm->tm_sec);
    return result;
}

void Log::print(int level, const char *file, int line, const char *message, ...) const
{
    if(level > logLevel) return;
    std::string str = "[" + string(getTime()) + "] ";

    if(file != 0 && line != 0)
        str += "["+ string(file) + ":" + to_string(line) + "] ";

    str += "[";
    switch(level)
    {
        case LOG_WARN:
            str += "WARN";
            break;
        case LOG_ERROR:
            str += "ERR";
            break;
        case LOG_FATAL:
            str += "FATAL";
            break;
        default:
            str += "INFO";
    }
    str += "]: ";
    str += message;
    if(str.back() != '\n')
        str += '\n';
    va_list args;
    va_start(args, message);
    vprintf(str.c_str(), args);
    va_end(args);
}

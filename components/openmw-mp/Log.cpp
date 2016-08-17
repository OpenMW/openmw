//
// Created by koncord on 15.08.16.
//

#include <cstdarg>
#include <iostream>
#include <cstring>
#include <ctime>
#include <cstdio>
#include <sstream>
#include "Log.hpp"

using namespace std;

Log *Log::sLog = NULL;

Log::Log(int logLevel) : logLevel(logLevel)
{

}

void Log::Create(int logLevel)
{
    if(sLog != NULL)
        return;
    sLog = new Log(logLevel);
}

void Log::Delete()
{
    if(sLog == NULL)
        return
    delete sLog;
    sLog = NULL;
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
    if(level < logLevel) return;
    std::stringstream sstr;
    sstr << "[" << getTime() << "] ";

    if(file != 0 && line != 0)
    {
        sstr << "[" << file << ":";
        sstr << line << "] ";
    }

    sstr << "[";
    switch(level)
    {
        case LOG_WARN:
            sstr << "WARN";
            break;
        case LOG_ERROR:
            sstr << "ERR";
            break;
        case LOG_FATAL:
            sstr << "FATAL";
            break;
        default:
            sstr << "INFO";
    }
    sstr << "]: ";
    sstr << message;
    char back = *sstr.str().rbegin();
    if(back != '\n')
        sstr << '\n';
    va_list args;
    va_start(args, message);
    vprintf(sstr.str().c_str(), args);
    va_end(args);
}

//
// Created by koncord on 15.08.16.
//

#include <cstdarg>
#include <iostream>
#include <cstring>
#include <ctime>
#include <cstdio>
#include <sstream>
#include <vector>
#include <boost/lexical_cast.hpp>
#include "Log.hpp"

using namespace std;

Log *Log::sLog = NULL;

Log::Log(int logLevel) : logLevel(logLevel)
{

}

void Log::Create(int logLevel)
{
    if (sLog != NULL)
        return;
    sLog = new Log(logLevel);
}

void Log::Delete()
{
    if (sLog == NULL)
        return;
    delete sLog;
    sLog = NULL;
}

const Log &Log::Get()
{
    return *sLog;
}

void Log::SetLevel(int level)
{
    sLog->logLevel = level;
}

const char* getTime()
{
    time_t t = time(0);
    struct tm *tm = localtime(&t);
    static char result[20];
    sprintf(result, "%.4d-%.2d-%.2d %.2d:%.2d:%.2d",
            1900 + tm->tm_year, tm->tm_mon + 1, tm->tm_mday,
            tm->tm_hour, tm->tm_min, tm->tm_sec);
    return result;
}

void Log::print(int level, bool hasPrefix, const char *file, int line, const char *message, ...) const
{
    if (level < logLevel) return;
    std::stringstream sstr;

    if (hasPrefix)
    {

        sstr << "[" << getTime() << "] ";

        if (file != 0 && line != 0)
        {
            sstr << "[" << file << ":";
            sstr << line << "] ";
        }

        sstr << "[";
        switch (level)
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

    }

    sstr << message;
    char back = *sstr.str().rbegin();
    if (back != '\n')
        sstr << '\n';
    va_list args;
    va_start(args, message);
    vector<char> buf((unsigned long) (vsnprintf(NULL, 0, sstr.str().c_str(), args) + 1));
    va_end(args);
    va_start(args, message);
    vsnprintf(buf.data(), buf.size(), sstr.str().c_str(), args);
    va_end(args);
    cout << buf.data() << flush;
}

string Log::getFilenameTimestamp()
{
    time_t rawtime = time(0);
    struct tm *timeinfo = localtime(&rawtime);
    char buffer[25];
    strftime(buffer, 25, "%Y-%m-%d-%H_%M_%S", timeinfo);
    std::string timestamp(buffer);
    return timestamp;
}

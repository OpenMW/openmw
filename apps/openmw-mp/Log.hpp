//
// Created by koncord on 15.08.16.
//

#ifndef OPENMW_LOG_HPP
#define OPENMW_LOG_HPP

#if defined(NOLOGS)
#define LOG_INIT(logLevel)
#define LOG_QUIT()
#define LOG_MESSAGE(level, msg, ...)
#define LOG_MESSAGE_SIMPLE(level, msg, ...)
#else
#define LOG_INIT(logLevel) Log::Create(logLevel)
#define LOG_QUIT() Log::Delete()
#define LOG_MESSAGE(level, msg, ...) Log::Get().print((level), (__FILE__), (__LINE__), (msg), __VA_ARGS__)
#define LOG_MESSAGE_SIMPLE(level, msg, ...) Log::Get().print((level), (0), (0), (msg), __VA_ARGS__)
#endif

class Log
{
    public:
        enum
        {
            INFO = 0,
            WARNING,
            ERROR,
            FATAL,
        };
        static void Create(int logLevel);
        static void Delete();
        static const Log &Get();
        void print(int level, const char *file, int line, const char *message, ...) const;
    private:
        Log(int logLevel);
        Log( const Log& ) = delete;
        Log& operator=( const Log& ) = delete;
        static Log * sLog;
        int logLevel;

};


#endif //OPENMW_LOG_HPP

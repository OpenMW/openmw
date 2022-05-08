#ifndef DEBUG_LOG_H
#define DEBUG_LOG_H

#include <mutex>
#include <iostream>

namespace Debug
{
    enum Level
    {
        Error = 1,
        Warning = 2,
        Info = 3,
        Verbose = 4,
        Debug = 5,
        Marker = Debug,

        NoLevel = 6 // Do not filter messages in this case
    };

    extern Level CurrentDebugLevel;
}

class Log
{
    static std::mutex sLock;

    std::unique_lock<std::mutex> mLock;
public:
    explicit Log(Debug::Level level)
        : mShouldLog(level <= Debug::CurrentDebugLevel)
    {
        // No need to hold the lock if there will be no logging anyway
        if (!mShouldLog)
            return;

        // Locks a global lock while the object is alive
        mLock = lock();

        // If the app has no logging system enabled, log level is not specified.
        // Show all messages without marker - we just use the plain cout in this case.
        if (Debug::CurrentDebugLevel == Debug::NoLevel)
            return;

        std::cout << static_cast<unsigned char>(level);
    }

    // Perfect forwarding wrappers to give the chain of objects to cout
    template<typename T>
    Log& operator<<(T&& rhs)
    {
        if (mShouldLog)
            std::cout << std::forward<T>(rhs);

        return *this;
    }

    ~Log()
    {
        if (mShouldLog)
            std::cout << std::endl;
    }

    static std::unique_lock<std::mutex> lock() { return std::unique_lock<std::mutex>(sLock); }

private:
    const bool mShouldLog;
};

#endif

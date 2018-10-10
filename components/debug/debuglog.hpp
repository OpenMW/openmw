#ifndef DEBUG_LOG_H
#define DEBUG_LOG_H

#include <mutex>
#include <iostream>

namespace Debug
{
    enum Level
    {
        NoLevel = 0,
        Error = 1,
        Warning = 2,
        Info = 3,
        Verbose = 4,
        Marker = Verbose
    };

    extern Level CurrentDebugLevel;
}

class Log
{
    static std::mutex sLock;

    std::unique_lock<std::mutex> mLock;
public:
    // Locks a global lock while the object is alive
    Log(Debug::Level level) :
    mLock(sLock),
    mLevel(level)
    {
        if (mLevel <= Debug::CurrentDebugLevel)
            std::cout << static_cast<unsigned char>(mLevel);
    }

    // Perfect forwarding wrappers to give the chain of objects to cout
    template<typename T>
    Log& operator<<(T&& rhs)
    {
        if (mLevel <= Debug::CurrentDebugLevel)
            std::cout << std::forward<T>(rhs);

        return *this;
    }

    ~Log()
    {
        if (mLevel <= Debug::CurrentDebugLevel)
            std::cout << std::endl;
    }

private:
    Debug::Level mLevel;
};

#endif

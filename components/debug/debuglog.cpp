#include "debuglog.hpp"
#include <mutex>

namespace Debug
{
    Level CurrentDebugLevel = Level::NoLevel;
}

static std::mutex sLock;

Log::Log(Debug::Level level) 
    : mShouldLog(level <= Debug::CurrentDebugLevel)
{
    // No need to hold the lock if there will be no logging anyway
    if (!mShouldLog)
        return;

    // Locks a global lock while the object is alive
    sLock.lock();

    // If the app has no logging system enabled, log level is not specified.
    // Show all messages without marker - we just use the plain cout in this case.
    if (Debug::CurrentDebugLevel == Debug::NoLevel)
        return;

    std::cout << static_cast<unsigned char>(level);
}

Log::~Log()
{
    if (!mShouldLog)
        return;

    std::cout << std::endl;
    sLock.unlock();
}

#ifndef DEBUG_LOG_H
#define DEBUG_LOG_H

#include <mutex>
#include <iostream>
#include <ctime>

#include <osg/io_utils>

namespace Debug
{
    enum Level
    {
        Error = 1,
        Warning = 2,
        Info = 3,
        Verbose = 4,
        Marker = Verbose,

        NoLevel = 5 // Do not filter messages in this case
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
        // If the app has no logging system enabled, log level is not specified.
        // Show all messages without marker - we just use the plain cout in this case.
        if (Debug::CurrentDebugLevel == Debug::NoLevel)
            return;

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

class Profiler
{
public:
    Profiler();

    void start(std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now())
    {
        mStarts.push_back(now);
    }

    std::chrono::steady_clock::duration finish(const char* name, std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now())
    {
        const auto start = mStarts.back();
        const auto duration = now - start;
        mStarts.pop_back();
        mRecords.push_back(Record {mStarts.size(), name, duration});
        return duration;
    }

    void save(const std::string& path = "profile." + std::to_string(std::time(nullptr)) + ".txt");

    static Profiler& instance()
    {
        static Profiler value;
        return value;
    }

private:
    struct Record
    {
        std::size_t mDepth;
        const char* mName;
        std::chrono::steady_clock::duration mDuration;
    };

    std::vector<std::chrono::steady_clock::time_point> mStarts;
    std::vector<Record> mRecords;
};

class ProfileScope
{
public:
    ProfileScope(const char* name)
        : mName(name)
    {
        ::Profiler::instance().start();
    }

    ~ProfileScope()
    {
        ::Profiler::instance().finish(mName);
    }

private:
    const char* mName;
};

class ProfileScopeWithLimit
{
public:
    ProfileScopeWithLimit(const char* name, std::chrono::steady_clock::duration max)
        : mName(name), mMax(max)
    {
        ::Profiler::instance().start();
    }

    ~ProfileScopeWithLimit()
    {
        const auto duration = ::Profiler::instance().finish(mName);
        if (duration > mMax)
            ::Log(::Debug::Verbose) << "Slow operation " << mName << " " << duration.count();
    }

private:
    const char* mName;
    std::chrono::steady_clock::duration mMax;
};


#endif

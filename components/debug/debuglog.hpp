#ifndef DEBUG_LOG_H
#define DEBUG_LOG_H

#include <filesystem>
#include <iostream>

namespace Debug
{
    enum Level : unsigned
    {
        Error = 1,
        Warning = 2,
        Info = 3,
        Verbose = 4,
        Debug = 5,
        All = 6,
    };
}

class Log
{
public:
    static Debug::Level sMinDebugLevel;
    static bool sWriteLevel;

    explicit Log(Debug::Level level);
    ~Log();

    template <typename T>
    Log& operator<<(const T& rhs)
    {
        if (mShouldLog)
            std::cout << rhs;

        return *this;
    }

    Log& operator<<(const std::filesystem::path& rhs);

    Log& operator<<(const std::u8string& rhs);

    Log& operator<<(std::u8string_view rhs);

    Log& operator<<(const char8_t* rhs);

private:
    const bool mShouldLog;
};

#endif

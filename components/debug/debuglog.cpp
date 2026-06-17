#include "debuglog.hpp"

#include <mutex>

#include <components/files/conversion.hpp>
#include <components/misc/strings/conversion.hpp>

static std::mutex sLock;

Debug::Level Log::sMinDebugLevel = Debug::All;
bool Log::sWriteLevel = false;

Log::Log(Debug::Level level)
    : mShouldLog(level <= sMinDebugLevel)
{
    // No need to hold the lock if there will be no logging anyway
    if (!mShouldLog)
        return;

    // Locks a global lock while the object is alive
    sLock.lock();

    if (!sWriteLevel)
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

Log& Log::operator<<(const std::filesystem::path& rhs)
{
    if (mShouldLog)
        std::cout << Files::pathToUnicodeString(rhs);

    return *this;
}

Log& Log::operator<<(const std::u8string& rhs)
{
    if (mShouldLog)
        std::cout << Misc::StringUtils::u8StringToString(rhs);

    return *this;
}

Log& Log::operator<<(const std::u8string_view rhs)
{
    if (mShouldLog)
        std::cout << Misc::StringUtils::u8StringToString(rhs);

    return *this;
}

Log& Log::operator<<(const char8_t* rhs)
{
    if (mShouldLog)
        std::cout << Misc::StringUtils::u8StringToString(rhs);

    return *this;
}

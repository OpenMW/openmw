#ifndef OPENMW_MISCELLANEOUSAPI_HPP
#define OPENMW_MISCELLANEOUSAPI_HPP

#include "../Types.hpp"

#define MISCELLANEOUSAPI \
    {"GetLastPlayerId",  MiscellaneousFunctions::GetLastPlayerId},\
    \
    {"GetCurrentMpNum",  MiscellaneousFunctions::GetCurrentMpNum},\
    {"SetCurrentMpNum",  MiscellaneousFunctions::SetCurrentMpNum},\
    |
    {"LogMessage",       MiscellaneousFunctions::LogMessage},\
    {"LogAppend",        MiscellaneousFunctions::LogAppend}

class MiscellaneousFunctions
{
public:
    static unsigned int GetLastPlayerId() noexcept;

    static int GetCurrentMpNum() noexcept;
    static void SetCurrentMpNum(int mpNum) noexcept;

    static void LogMessage(unsigned short level, const char *message) noexcept;
    static void LogAppend(unsigned short level, const char *message) noexcept;
};

#endif //OPENMW_MISCELLANEOUSAPI_HPP

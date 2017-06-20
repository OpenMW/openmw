#ifndef OPENMW_SETTINGSAPI_HPP
#define OPENMW_SETTINGSAPI_HPP

#include "../Types.hpp"

#define SETTINGSAPI \
    {"SetConsoleAllow",  SettingFunctions::SetConsoleAllow},\
    {"SetDifficulty",    SettingFunctions::SetDifficulty},\
    \
    {"SendSettings",     SettingFunctions::SendSettings}

class SettingFunctions
{
public:
    static void SetConsoleAllow(unsigned short pid, bool state);
    static void SetDifficulty(unsigned short pid, int difficulty);

    static void SendSettings(unsigned short pid) noexcept;
};

#endif //OPENMW_SETTINGSAPI_HPP

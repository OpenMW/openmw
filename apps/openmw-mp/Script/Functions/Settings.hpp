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

    /**
    * \brief Set whether the console is allowed for a player.
    *
    * This changes the console permission for that player in the server memory, but does not
    * by itself send a packet.
    *
    * \param pid The player id.
    * \param bool The console permission state.
    * \return void
    */
    static void SetConsoleAllow(unsigned short pid, bool state);

    /**
    * \brief Set the difficulty for a player.
    *
    * This changes the difficulty for that player in the server memory, but does not by itself
    * send a packet.
    *
    * \param pid The player id.
    * \param bool The difficulty.
    * \return void
    */
    static void SetDifficulty(unsigned short pid, int difficulty);

    /**
    * \brief Send a PlayerSettings packet to the player affected by it.
    *
    * \param pid The player ID to send it to.
    * \return void
    */
    static void SendSettings(unsigned short pid) noexcept;
};

#endif //OPENMW_SETTINGSAPI_HPP

#ifndef OPENMW_MECHANICSAPI_HPP
#define OPENMW_MECHANICSAPI_HPP

#include "../Types.hpp"

#define MECHANICSAPI \
    {"IsWerewolf",          MechanicsFunctions::IsWerewolf},\
    \
    {"SetWerewolfState",    MechanicsFunctions::SetWerewolfState},\
    \
    {"SendShapeshift",      MechanicsFunctions::SendShapeshift},\
    \
    {"Jail",                MechanicsFunctions::Jail},\
    {"Resurrect",           MechanicsFunctions::Resurrect}

class MechanicsFunctions
{
public:

    /**
    * \brief Check whether a player is a werewolf.
    *
    * This is based on the last PlayerShapeshift packet received or sent for that player.
    *
    * \param pid The player id.
    * \return The werewolf state.
    */
    static bool IsWerewolf(unsigned short pid) noexcept;

    /**
    * \brief Set the werewolf state of a player.
    *
    * This changes the werewolf state recorded for that player in the server memory, but
    * does not by itself send a packet.
    *
    * \param pid The player id.
    * \param bool The new werewolf state.
    * \return void
    */
    static void SetWerewolfState(unsigned short pid, bool isWerewolf);

    /**
    * \brief Send a PlayerShapeshift packet about a player.
    *
    * This sends the packet to all players connected to the server. It is currently used
    * only to communicate werewolf states.
    *
    * \param pid The player id.
    * \return void
    */
    static void SendShapeshift(unsigned short pid);

    /**
    * \brief Send a PlayerJail packet about a player.
    *
    * This is similar to the player being jailed by a guard, but provides extra parameters for
    * increased flexibility.
    *
    * It is only sent to the player being jailed, as the other players will be informed of the
    * jailing's actual consequences via other packets sent by the affected client.
    *
    * \param pid The player id.
    * \param jailDays The number of days to spend jailed, where each day affects one skill point.
    * \param ignoreJailTeleportation Whether the player being teleported to the nearest jail
    *                                marker should be overridden.
    * \param ignoreJailSkillIncrease Whether the player's Sneak and Security skills should be
    *                                prevented from increasing as a result of the jailing,
    *                                overriding default behavior.
    * \param jailProgressText The text that should be displayed while jailed.
    * \param jailEndText The text that should be displayed once the jailing period is over.
    * \return void
    */
    static void Jail(unsigned short pid, int jailDays, bool ignoreJailTeleportation = false, bool ignoreJailSkillIncreases = false,
                     const char* jailProgressText = "", const char* jailEndText = "") noexcept;

    /**
    * \brief Send a PlayerResurrect packet about a player.
    *
    * This sends the packet to all players connected to the server.
    *
    * \param pid The player id.
    * \param type The type of resurrection (0 for REGULAR, 1 for IMPERIAL_SHRINE,
    *             2 for TRIBUNAL_TEMPLE).
    * \return void
    */
    static void Resurrect(unsigned short pid, unsigned int type) noexcept;
};

#endif //OPENMW_MECHANICSAPI_HPP

//
// Created by koncord on 24.01.16.
//

#ifndef SOURCEPAWN_SCRIPTFUNCTIONS_HPP
#define SOURCEPAWN_SCRIPTFUNCTIONS_HPP

#include <RakNetTypes.h>
//#include <amx/amx.h>
#include <tuple>
#include <apps/openmw-mp/Player.hpp>
#include "ScriptFunction.hpp"
#include "Types.hpp"

#define GET_PLAYER(pid, pl, retvalue) \
     pl = Players::GetPlayer(pid); \
     if (player == 0) {\
        fprintf(stderr, "%s: Player with pid \'%d\' not found\n", __PRETTY_FUNCTION__, pid);\
        /*ScriptFunctions::StopServer(1);*/ \
        return retvalue;\
}


class ScriptFunctions
{
public:

    static void GetArguments(std::vector<boost::any> &params, va_list args, const std::string &def);

    static void StopServer(int code) noexcept;

    static void MakePublic(ScriptFunc _public, const char *name, char ret_type, const char *def) noexcept;
    static boost::any CallPublic(const char *name, ...) noexcept;

    static void GetPos(unsigned short pid, float *x, float *y, float *z) noexcept;
    static void SetPos(unsigned short pid, float x, float y, float z) noexcept;


    static void GetAngle(unsigned short pid, float *x, float *y, float *z) noexcept;
    static void SetAngle(unsigned short pid, float x, float y, float z) noexcept;

    static void SetCell(unsigned short pid, const char *name) noexcept;
    static const char *GetCell(unsigned short pid) noexcept;

    static bool IsInInterior(unsigned short pid) noexcept;

    static void SetName(unsigned short pid, const char *name) noexcept;
    static const char *GetName(unsigned short pid) noexcept;

    static void SetBirthsign(unsigned short pid, const char *name) noexcept;
    static const char *GetBirthsign(unsigned short pid) noexcept;

    static void SetRace(unsigned short pid, const char *race) noexcept;
    static const char *GetRace(unsigned short pid) noexcept;

    static void SetHead(unsigned short pid, const char *head) noexcept;
    static const char *GetHead(unsigned short pid) noexcept;

    static void SetHairstyle(unsigned short pid, const char *style) noexcept;
    static const char *GetHairstyle(unsigned short pid) noexcept;

    static void SetIsMale(unsigned short pid, int male) noexcept;
    static int GetIsMale(unsigned short pid) noexcept;

    static float GetHealth(unsigned short pid) noexcept;
    static void SetHealth(unsigned short pid, float health) noexcept;
    static float GetCurrentHealth(unsigned short pid) noexcept;
    static void SetCurrentHealth(unsigned short pid, float health) noexcept;

    static float GetMagicka(unsigned short pid) noexcept;
    static void SetMagicka(unsigned short pid, float magicka) noexcept;
    static float GetCurrentMagicka(unsigned short pid) noexcept;
    static void SetCurrentMagicka(unsigned short pid, float magicka) noexcept;

    static float GetFatigue(unsigned short pid) noexcept;
    static void SetFatigue(unsigned short pid, float fatigue) noexcept;
    static float GetCurrentFatigue(unsigned short pid) noexcept;
    static void SetCurrentFatigue(unsigned short pid, float fatigue) noexcept;

    static int GetAttribute(unsigned short pid, unsigned short attribute) noexcept;
    static void SetAttribute(unsigned short pid, unsigned short attribute, int value) noexcept;
    static int GetCurrentAttribute(unsigned short pid, unsigned short attribute) noexcept;
    static void SetCurrentAttribute(unsigned short pid, unsigned short attribute, int value) noexcept;

    static int GetSkill(unsigned short pid, unsigned short skill) noexcept;
    static void SetSkill(unsigned short pid, unsigned short skill, int value) noexcept;
    static int GetCurrentSkill(unsigned short pid, unsigned short skill) noexcept;
    static void SetCurrentSkill(unsigned short pid, unsigned short skill, int value) noexcept;

    static int GetIncreaseSkill(unsigned short pid, unsigned int pos) noexcept;
    static void SetIncreaseSkill(unsigned short pid, unsigned int pos, int value) noexcept;

    static void Resurrect(unsigned short pid);

    //static void AddItem(unsigned short pid, const char* itemName, unsigned short count) noexcept;
    //static void RemoveItem(unsigned short pid, const char* itemName, unsigned short count) noexcept;
    //static void GetItemCount(unsigned short pid, const char* itemName) noexcept;
    static void EquipItem(unsigned short pid, unsigned short slot, const char* itemName, unsigned short count) noexcept;
    static void UnequipItem(unsigned short pid, unsigned short slot) noexcept;
    static bool HasItemEquipped(unsigned short pid, const char* itemName);
    static const char *GetItemSlot(unsigned short pid, unsigned short slot) noexcept;

    static void SendMessage(unsigned short pid, const char *message, bool broadcast) noexcept;
    static void CleanChat(unsigned short pid);
    static void CleanChat();
    static void SetCharGenStage(unsigned short pid, int start, int end) noexcept;

    /**
     * \brief Create timer
     * \param callback
     * \param msec
     * \return return timer id
     */
    static int CreateTimer(ScriptFunc callback, int msec) noexcept;
    static int CreateTimerEx(ScriptFunc callback, int msec, const char *types, ...) noexcept;


    static void StartTimer(int timerId) noexcept;
    static void StopTimer(int timerId) noexcept;
    static void RestartTimer(int timerId, int msec) noexcept;
    static void FreeTimer(int timerId) noexcept;
    static bool IsTimerElapsed(int timerId) noexcept;

    static void Kick(unsigned short pid) noexcept;

    static constexpr ScriptFunctionData functions[]{
            {"CreateTimer",         ScriptFunctions::CreateTimer},
            {"CreateTimerEx",       reinterpret_cast<Function<void>>(ScriptFunctions::CreateTimerEx)},
            {"MakePublic",          ScriptFunctions::MakePublic},
            {"CallPublic",          reinterpret_cast<Function<void>>(ScriptFunctions::CallPublic)},


            {"StartTimer",          ScriptFunctions::StartTimer},
            {"StopTimer",           ScriptFunctions::StopTimer},
            {"RestartTimer",        ScriptFunctions::RestartTimer},
            {"FreeTimer",           ScriptFunctions::FreeTimer},
            {"IsTimerElapsed",      ScriptFunctions::IsTimerElapsed},

            {"StopServer",          ScriptFunctions::StopServer},

            {"GetPos",              ScriptFunctions::GetPos},
            {"SetPos",              ScriptFunctions::SetPos},

            {"GetAngle",            ScriptFunctions::GetAngle},
            {"SetAngle",            ScriptFunctions::SetAngle},

            {"GetCell",             ScriptFunctions::GetCell},
            {"SetCell",             ScriptFunctions::SetCell},
            {"IsInInterior",        ScriptFunctions::IsInInterior},

            {"GetName",             ScriptFunctions::GetName},
            {"SetName",             ScriptFunctions::SetName},

            {"GetRace",             ScriptFunctions::GetRace},
            {"SetRace",             ScriptFunctions::SetRace},

            {"GetHead",             ScriptFunctions::GetHead},
            {"SetHead",             ScriptFunctions::SetHead},

            {"GetHair",             ScriptFunctions::GetHairstyle},
            {"SetHair",             ScriptFunctions::SetHairstyle},

            {"GetIsMale",           ScriptFunctions::GetIsMale},
            {"SetIsMale",           ScriptFunctions::SetIsMale},

            {"GetBirthsign",        ScriptFunctions::GetBirthsign},
            {"SetBirthsign",        ScriptFunctions::SetBirthsign},

            {"GetAttribute",        ScriptFunctions::GetAttribute},
            {"SetAttribute",        ScriptFunctions::SetAttribute},
            {"GetCurrentAttribute", ScriptFunctions::GetCurrentAttribute},
            {"SetCurrentAttribute", ScriptFunctions::SetCurrentAttribute},
            {"GetSkill",            ScriptFunctions::GetSkill},
            {"SetSkill",            ScriptFunctions::SetSkill},
            {"GetCurrentSkill",     ScriptFunctions::GetCurrentSkill},
            {"SetCurrentSkill",     ScriptFunctions::SetCurrentSkill},

            {"GetHealth",           ScriptFunctions::GetHealth},
            {"SetHealth",           ScriptFunctions::SetHealth},
            {"GetCurrentHealth",    ScriptFunctions::GetCurrentHealth},
            {"SetCurrentHealth",    ScriptFunctions::SetCurrentHealth},

            {"GetMagicka",          ScriptFunctions::GetMagicka},
            {"SetMagicka",          ScriptFunctions::SetMagicka},
            {"GetCurrentMagicka",   ScriptFunctions::GetCurrentMagicka},
            {"SetCurrentMagicka",   ScriptFunctions::SetCurrentMagicka},

            {"SetFatigue",          ScriptFunctions::SetFatigue},
            {"GetFatigue",          ScriptFunctions::GetFatigue},
            {"SetCurrentFatigue",   ScriptFunctions::SetCurrentFatigue},
            {"GetCurrentFatigue",   ScriptFunctions::GetCurrentFatigue},

//            {"SetClass",          ScriptFunctions::SetClass},
//            {"GetClass",          ScriptFunctions::GetClass},
            {"GetIncreaseSkill",    ScriptFunctions::GetIncreaseSkill},
            {"SetIncreaseSkill",    ScriptFunctions::SetIncreaseSkill},

//            {"Cast",              ScriptFunctions::Cast},

//            {"AddItem",           ScriptFunctions::AddItem},
//            {"RemoveItem",        ScriptFunctions::RemoveItem},
//            {"GetItemCount",      ScriptFunctions::GetItemCount},
            {"EquipItem",           ScriptFunctions::EquipItem},
            {"UnequipItem",         ScriptFunctions::UnequipItem},
            {"GetItemSlot",         ScriptFunctions::GetItemSlot},
            {"HasItemEquipped",     ScriptFunctions::HasItemEquipped},
            {"SendMessage",         ScriptFunctions::SendMessage},
            {"SetCharGenStage",     ScriptFunctions::SetCharGenStage},
            {"Resurrect",           ScriptFunctions::Resurrect},

            {"Kick",                ScriptFunctions::Kick},
    };

    static constexpr ScriptCallbackData callbacks[]{
            {"Main",                  Function<int, int, int>()},
            {"OnServerInit",          Function<void>()},
            {"OnServerExit",          Function<void, bool>()},
            {"OnPlayerConnect",       Function<bool, unsigned short>()},
            {"OnPlayerDisconnect",    Function<void, unsigned short>()},
            {"OnPlayerDeath",         Function<void, unsigned short>()},
            {"OnPlayerResurrect",     Function<void, unsigned short>()},
            {"OnPlayerChangeCell",    Function<void, unsigned short>()},
            {"OnPlayerUpdateEquiped", Function<void, unsigned short>()},
            {"OnPlayerSendMessage",   Function<bool, unsigned short, const char*>()},
            {"OnPlayerEndCharGen",    Function<void, unsigned short>()}
    };
};

#endif //SOURCEPAWN_SCRIPTFUNCTIONS_HPP
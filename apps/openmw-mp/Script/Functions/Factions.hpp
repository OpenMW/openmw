#ifndef OPENMW_FACTIONAPI_HPP
#define OPENMW_FACTIONAPI_HPP

#define FACTIONAPI \
    {"InitializeFactionChanges", FactionFunctions::InitializeFactionChanges},\
    \
    {"GetFactionChangesSize",    FactionFunctions::GetFactionChangesSize},\
    {"GetFactionChangesAction",  FactionFunctions::GetFactionChangesAction},\
    \
    {"GetFactionId",             FactionFunctions::GetFactionId},\
    {"GetFactionRank",           FactionFunctions::GetFactionRank},\
    {"GetFactionExpelledState",  FactionFunctions::GetFactionExpelledState},\
    {"GetFactionReputation",     FactionFunctions::GetFactionReputation},\
    \
    {"SetFactionChangesAction",  FactionFunctions::SetFactionChangesAction},\
    {"SetFactionId",             FactionFunctions::SetFactionId},\
    {"SetFactionRank",           FactionFunctions::SetFactionRank},\
    {"SetFactionExpulsionState", FactionFunctions::SetFactionExpulsionState},\
    {"SetFactionReputation",     FactionFunctions::SetFactionReputation},\
    \
    {"AddFaction",               FactionFunctions::AddFaction},\
    \
    {"SendFactionChanges",       FactionFunctions::SendFactionChanges}

class FactionFunctions
{
public:

    static void InitializeFactionChanges(unsigned short pid) noexcept;

    static unsigned int GetFactionChangesSize(unsigned short pid) noexcept;
    static unsigned char GetFactionChangesAction(unsigned short pid) noexcept;

    static const char *GetFactionId(unsigned short pid, unsigned int i) noexcept;
    static int GetFactionRank(unsigned short pid, unsigned int i) noexcept;
    static bool GetFactionExpelledState(unsigned short pid, unsigned int i) noexcept;
    static int GetFactionReputation(unsigned short pid, unsigned int i) noexcept;

    static void SetFactionChangesAction(unsigned short pid, unsigned char action) noexcept;
    static void SetFactionId(const char* factionId) noexcept;
    static void SetFactionRank(unsigned int rank) noexcept;
    static void SetFactionExpulsionState(bool isExpelled) noexcept;
    static void SetFactionReputation(int reputation) noexcept;

    static void AddFaction(unsigned short pid) noexcept;

    static void SendFactionChanges(unsigned short pid, bool toOthers = false) noexcept;
private:

};

#endif //OPENMW_FACTIONAPI_HPP

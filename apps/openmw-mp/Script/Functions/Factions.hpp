#ifndef OPENMW_FACTIONAPI_HPP
#define OPENMW_FACTIONAPI_HPP

#define FACTIONAPI \
    {"GetFactionChangesSize",   FactionFunctions::GetFactionChangesSize},\
    {"GetFactionChangesAction", FactionFunctions::GetFactionChangesAction},\
    \
    {"AddFaction",              FactionFunctions::AddFaction},\
    \
    {"GetFactionId",            FactionFunctions::GetFactionId},\
    {"GetFactionRank",          FactionFunctions::GetFactionRank},\
    {"GetFactionExpelledState", FactionFunctions::GetFactionExpelledState},\
    \
    {"SendFactionChanges",      FactionFunctions::SendFactionChanges}

class FactionFunctions
{
public:

    static unsigned int GetFactionChangesSize(unsigned short pid) noexcept;
    static unsigned char GetFactionChangesAction(unsigned short pid) noexcept;

    static void AddFaction(unsigned short pid, const char* factionId, unsigned int rank, bool isExpelled) noexcept;

    static const char *GetFactionId(unsigned short pid, unsigned int i) noexcept;
    static int GetFactionRank(unsigned short pid, unsigned int i) noexcept;
    static bool GetFactionExpelledState(unsigned short pid, unsigned int i) noexcept;

    static void SendFactionChanges(unsigned short pid) noexcept;
private:

};

#endif //OPENMW_FACTIONAPI_HPP

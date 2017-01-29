#ifndef OPENMW_WORLD_HPP
#define OPENMW_WORLD_HPP

#define WORLDFUNCTIONS \
    {"CreateWorldEvent",     WorldFunctions::CreateWorldEvent},\
    \
    {"AddWorldObject",       WorldFunctions::AddWorldObject},\
    {"SetWorldEventCell",    WorldFunctions::SetWorldEventCell},\
    \
    {"SetObjectRefId",       WorldFunctions::SetObjectRefId},\
    {"SetObjectRefNumIndex", WorldFunctions::SetObjectRefNumIndex},\
    {"SetObjectPosition",    WorldFunctions::SetObjectPosition},\
    \
    {"GetObjectChangesSize", WorldFunctions::GetObjectChangesSize},\
    \
    {"GetObjectRefId",       WorldFunctions::GetObjectRefId},\
    {"GetObjectRefNumIndex", WorldFunctions::GetObjectRefNumIndex},\
    \
    {"SendObjectDelete",     WorldFunctions::SendObjectDelete},\
    {"SendObjectPlace",      WorldFunctions::SendObjectPlace},\
    \
    {"SetHour",              WorldFunctions::SetHour},\
    {"SetMonth",             WorldFunctions::SetMonth},\
    {"SetDay",               WorldFunctions::SetDay}

class WorldFunctions
{
public:

    static void CreateWorldEvent(unsigned short pid) noexcept;

    static void AddWorldObject() noexcept;
    static void SetWorldEventCell(const char* cellDescription) noexcept;

    static void SetObjectRefId(unsigned int i, const char* refId) noexcept;
    static void SetObjectRefNumIndex(unsigned int i, int refNumIndex) noexcept;
    static void SetObjectPosition(unsigned int i, double x, double y, double z) noexcept;

    static unsigned int GetObjectChangesSize() noexcept;

    static const char *GetObjectRefId(unsigned int i) noexcept;
    static int GetObjectRefNumIndex(unsigned int i) noexcept;

    static void SendObjectDelete() noexcept;
    static void SendObjectPlace() noexcept;

    static void SetHour(unsigned short pid, double hour) noexcept;
    static void SetMonth(unsigned short pid, int month) noexcept;
    static void SetDay(unsigned short pid, int day) noexcept;
};


#endif //OPENMW_WORLD_HPP

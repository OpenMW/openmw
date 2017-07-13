#ifndef OPENMW_MECHANICSAPI_HPP
#define OPENMW_MECHANICSAPI_HPP

#include "../Types.hpp"

#define MECHANICSAPI \
    {"Jail",                MechanicsFunctions::Jail},\
    \
    {"Resurrect",           MechanicsFunctions::Resurrect}

class MechanicsFunctions
{
public:
    static void Jail(unsigned short pid, int jailDays, bool ignoreJailTeleportation = false, bool ignoreJailSkillIncreases = false,
                     const char* jailProgressText = "", const char* jailEndText = "") noexcept;

    static void Resurrect(unsigned short pid, unsigned int type) noexcept;
};

#endif //OPENMW_MECHANICSAPI_HPP

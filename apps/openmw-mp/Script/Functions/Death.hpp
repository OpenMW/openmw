#ifndef OPENMW_DEATHAPI_HPP
#define OPENMW_DEATHAPI_HPP

#include "../Types.hpp"

#define DEATHAPI \
    {"SetDeathPenaltyJailDays",  DeathFunctions::SetDeathPenaltyJailDays},\
    \
    {"Resurrect",                DeathFunctions::Resurrect}

class DeathFunctions
{
public:
    static void SetDeathPenaltyJailDays(unsigned short pid, int days) noexcept;

    static void Resurrect(unsigned short pid, unsigned int type) noexcept;
};

#endif //OPENMW_DEATHAPI_HPP

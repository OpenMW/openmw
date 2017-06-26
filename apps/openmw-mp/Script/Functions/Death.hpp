#ifndef OPENMW_DEATHAPI_HPP
#define OPENMW_DEATHAPI_HPP

#include "../Types.hpp"

#define DEATHAPI \
    {"SetResurrectType",  DeathFunctions::SetResurrectType},\
    \
    {"SendResurrect",     DeathFunctions::SendResurrect}

class DeathFunctions
{
public:
    static void SetResurrectType(unsigned short pid, unsigned int type);

    static void SendResurrect(unsigned short pid) noexcept;
};

#endif //OPENMW_DEATHAPI_HPP

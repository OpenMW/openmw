#ifndef OPENMW_DEATHAPI_HPP
#define OPENMW_DEATHAPI_HPP

#include "../Types.hpp"

#define DEATHAPI \
    {"Resurrect",     DeathFunctions::Resurrect}

class DeathFunctions
{
public:
    static void Resurrect(unsigned short pid, unsigned int type) noexcept;
};

#endif //OPENMW_DEATHAPI_HPP

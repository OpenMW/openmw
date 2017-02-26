#ifndef OPENMW_MISCELLANEOUSAPI_HPP
#define OPENMW_MISCELLANEOUSAPI_HPP

#include "../Types.hpp"

#define MISCELLANEOUSAPI \
    {"GetLastPlayerId",  MiscellaneousFunctions::GetLastPlayerId}


class MiscellaneousFunctions
{
public:
    static unsigned int GetLastPlayerId() noexcept;
};

#endif //OPENMW_MISCELLANEOUSAPI_HPP

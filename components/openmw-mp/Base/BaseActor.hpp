#ifndef OPENMW_BASEACTOR_HPP
#define OPENMW_BASEACTOR_HPP

#include <components/esm/loadcell.hpp>

namespace mwmp
{
    class BaseActor
    {
    public:

        BaseActor()
        {

        }

        unsigned int movementFlags;
        char movementAnim;
        char drawState;
        bool isFlying;

        ESM::Position position;
        ESM::Position direction;
        ESM::Cell cell;
    };
}

#endif //OPENMW_BASEACTOR_HPP

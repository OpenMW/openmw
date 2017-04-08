#ifndef OPENMW_BASEACTOR_HPP
#define OPENMW_BASEACTOR_HPP

#include <components/esm/loadcell.hpp>
#include <components/openmw-mp/Base/BaseStructs.hpp>

namespace mwmp
{
    class BaseActor
    {
    public:

        BaseActor()
        {

        }

        char drawState;
        bool isFlying;

        ESM::Position position;
        ESM::Cell cell;

        float headPitch;
        float headYaw;

        Animation animation;
        bool hasAnimation;

        AnimStates animStates;
        bool hasAnimStates;

        Movement movement;
        bool hasMovement;
    };
}

#endif //OPENMW_BASEACTOR_HPP

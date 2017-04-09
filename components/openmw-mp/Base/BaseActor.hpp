#ifndef OPENMW_BASEACTOR_HPP
#define OPENMW_BASEACTOR_HPP

#include <components/esm/loadcell.hpp>
#include <components/openmw-mp/Base/BaseStructs.hpp>
#include <RakNetTypes.h>

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

    class ActorList
    {
    public:

        ActorList()
        {

        }

        RakNet::RakNetGUID guid;

        std::vector<BaseActor> baseActors;
        unsigned int count;

        ESM::Cell cell;
    };
}

#endif //OPENMW_BASEACTOR_HPP

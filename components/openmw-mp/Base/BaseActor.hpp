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

        std::string refId;
        int refNumIndex;
        int mpNum;

        ESM::Position position;
        ESM::Position direction;
        
        ESM::Cell cell;

        unsigned int movementFlags;
        char drawState;
        bool isFlying;

        float headPitch;
        float headYaw;

        Animation animation;
    };

    class BaseActorList
    {
    public:

        BaseActorList()
        {

        }

        enum ACTOR_ACTION
        {
            SET = 0,
            ADD = 1,
            REMOVE = 2,
            REQUEST = 3
        };

        RakNet::RakNetGUID guid;

        std::vector<BaseActor> baseActors;
        unsigned int count;

        ESM::Cell cell;

        unsigned char action; // 0 - Clear and set in entirety, 1 - Add item, 2 - Remove item, 3 - Request items
    };
}

#endif //OPENMW_BASEACTOR_HPP

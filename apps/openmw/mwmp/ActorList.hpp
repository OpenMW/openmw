#ifndef OPENMW_ACTORLIST_HPP
#define OPENMW_ACTORLIST_HPP

#include <components/openmw-mp/Base/BaseActor.hpp>
#include "../mwworld/cellstore.hpp"
#include <RakNetTypes.h>

#include "LocalActor.hpp"

namespace mwmp
{
    class Networking;
    class ActorList : public BaseActorList
    {
    public:

        ActorList();
        virtual ~ActorList();

        void reset();
        void addActor(BaseActor baseActor);
        void addActor(LocalActor localActor);

        void addPositionActor(LocalActor localActor);
        void addAnimFlagsActor(LocalActor localActor);

        void sendPositionActors();
        void sendAnimFlagsActors();

        void editActorsInCell(MWWorld::CellStore* cellStore);

        void sendActorsInCell(MWWorld::CellStore* cellStore);

    private:
        Networking *getNetworking();

        std::vector<BaseActor> positionActors;
        std::vector<BaseActor> animFlagsActors;
    };
}

#endif //OPENMW_ACTORLIST_HPP

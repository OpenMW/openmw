#ifndef OPENMW_WORLDEVENT_HPP
#define OPENMW_WORLDEVENT_HPP

#include <components/openmw-mp/Base/BaseEvent.hpp>
#include "../mwworld/cellstore.hpp"
#include <RakNetTypes.h>

namespace mwmp
{
    class Networking;
    class WorldEvent : public BaseEvent
    {
    public:

        WorldEvent();
        virtual ~WorldEvent();

        void addObject(WorldObject worldObject);

        void sendContainers(MWWorld::CellStore* cellStore);

        void editContainers(MWWorld::CellStore* cellStore);
        void placeObjects(MWWorld::CellStore* cellStore);
        void deleteObjects(MWWorld::CellStore* cellStore);
        void lockObjects(MWWorld::CellStore* cellStore);
        void unlockObjects(MWWorld::CellStore* cellStore);
        void scaleObjects(MWWorld::CellStore* cellStore);
        void moveObjects(MWWorld::CellStore* cellStore);
        void rotateObjects(MWWorld::CellStore* cellStore);
        void animateObjects(MWWorld::CellStore* cellStore);
        void activateDoors(MWWorld::CellStore* cellStore);

        void setLocalShorts(MWWorld::CellStore* cellStore);
        void setLocalFloats(MWWorld::CellStore* cellStore);
        void setMemberShorts();
        void setGlobalShorts();

        void playMusic();
        void playVideo();

    private:
        Networking *getNetworking();

    };
}

#endif //OPENMW_WORLDEVENT_HPP

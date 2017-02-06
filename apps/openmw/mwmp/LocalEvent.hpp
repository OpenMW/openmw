#ifndef OPENMW_LOCALEVENT_HPP
#define OPENMW_LOCALEVENT_HPP

#include <components/openmw-mp/Base/BaseEvent.hpp>
#include "../mwworld/cellstore.hpp"
#include <RakNetTypes.h>

namespace mwmp
{
    class Networking;
    class LocalEvent : public BaseEvent
    {
    public:

        LocalEvent(RakNet::RakNetGUID guid);
        virtual ~LocalEvent();

        void addObject(WorldObject worldObject);
        void addContainerItem(ContainerItem containerItem);

        void editContainer(MWWorld::CellStore* cellStore);
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

#endif //OPENMW_LOCALEVENT_HPP

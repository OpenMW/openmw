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

        void sendActors(MWWorld::CellStore* cellStore);
        void sendContainers(MWWorld::CellStore* cellStore);

        void sendObjectPlace(MWWorld::Ptr ptr);
        void sendObjectDelete(MWWorld::Ptr ptr);
        void sendObjectLock(MWWorld::Ptr ptr, int lockLevel);
        void sendObjectUnlock(MWWorld::Ptr ptr);
        void sendObjectScale(MWWorld::Ptr ptr, int scale);
        void sendObjectAnimPlay(MWWorld::Ptr ptr, std::string group, int mode);
        void sendDoorState(MWWorld::Ptr ptr, int state);
        void sendMusicPlay(std::string filename);
        void sendVideoPlay(std::string filename, bool allowSkipping);
        void sendScriptLocalShort(MWWorld::Ptr ptr, int index, int shortVal);
        void sendScriptLocalFloat(MWWorld::Ptr ptr, int index, float floatVal);
        void sendScriptMemberShort(std::string refId, int index, int shortVal);
        void sendScriptGlobalShort(std::string varName, int shortVal);

        void editActors(MWWorld::CellStore* cellStore);
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

#ifndef OPENMW_BASEEVENT_HPP
#define OPENMW_BASEEVENT_HPP

#include <components/esm/loadcell.hpp>
#include <components/esm/cellref.hpp>
#include <RakNetTypes.h>

namespace mwmp
{
    struct ContainerItem
    {
        std::string refId;
        int count;
        int charge;

        int actionCount;

        inline bool operator==(const ContainerItem& rhs)
        {
            return refId == rhs.refId && count == rhs.count && charge == rhs.charge;
        }
    };

    struct ContainerChanges
    {
        std::vector<ContainerItem> items;
        unsigned int count;
    };

    struct WorldObject
    {
        std::string refId;
        int refNumIndex;
        int count;
        int charge;
        int goldValue;
        ESM::Position pos;

        int doorState;
        int lockLevel;
        float scale;

        std::string filename;
        bool allowSkipping;

        std::string animGroup;
        int animMode;

        int index;
        int shortVal;
        float floatVal;
        std::string varName;

        ContainerChanges containerChanges;
    };

    struct ObjectChanges
    {
        std::vector<WorldObject> objects;
        unsigned int count;
    };

    class BaseEvent
    {
    public:

        BaseEvent(RakNet::RakNetGUID guid) : guid(guid)
        {

        }

        BaseEvent()
        {

        }

        enum WORLD_ACTION
        {
            SET = 0,
            ADD = 1,
            REMOVE = 2,
            REQUEST = 3
        };

        RakNet::RakNetGUID guid;
        ObjectChanges objectChanges;

        ESM::Cell cell;

        unsigned char action; // 0 - Clear and set in entirety, 1 - Add item, 2 - Remove item, 3 - Request items
    };
}

#endif //OPENMW_BASEEVENT_HPP

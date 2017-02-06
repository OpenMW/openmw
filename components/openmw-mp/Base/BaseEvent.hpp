#ifndef OPENMW_BASEEVENT_HPP
#define OPENMW_BASEEVENT_HPP

#include <components/esm/loadcell.hpp>
#include <components/esm/cellref.hpp>
#include <RakNetTypes.h>

namespace mwmp
{
    struct WorldObject
    {
        std::string refId;
        int refNumIndex;
        int count;
        int charge;
        int goldValue;
        ESM::Position pos;

        int state;
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
    };

    struct ContainerItem
    {
        std::string refId;
        int count;
        int charge;
        int goldValue;

        std::string owner;
        int actionCount;

        inline bool operator==(const ContainerItem& rhs)
        {
            return refId == rhs.refId && count == rhs.count && charge == rhs.charge && goldValue && rhs.goldValue;
        }
    };

    struct ObjectChanges
    {
        std::vector<WorldObject> objects;
        unsigned int count;
    };

    struct ContainerChanges
    {
        std::vector<ContainerItem> items;
        unsigned int count;

        enum CONTAINER_ACTION
        {
            SET = 0,
            ADD = 1,
            REMOVE = 2
        };

        int action; // 0 - Clear and set in entirety, 1 - Add item, 2 - Remove item
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

        RakNet::RakNetGUID guid;
        ObjectChanges objectChanges;
        ContainerChanges containerChanges;

        ESM::Cell cell;
    };
}

#endif //OPENMW_BASEEVENT_HPP

#ifndef OPENMW_WORLDEVENT_HPP
#define OPENMW_WORLDEVENT_HPP

#include <components/esm/loadcell.hpp>
#include <components/esm/cellref.hpp>
#include <RakNetTypes.h>

namespace mwmp
{
    struct WorldObject
    {
        std::string refId;
        int refNumIndex;
        int goldValue;
        int count;
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
        int health;
        inline bool operator==(const ContainerItem& rhs)
        {
            return refId == rhs.refId && count == rhs.count && health == rhs.health;
        }
    };

    class WorldEvent
    {
    public:

        WorldEvent(RakNet::RakNetGUID guid) : guid(guid)
        {

        }

        WorldEvent()
        {

        }

        struct ObjectChanges
        {
            std::vector<WorldObject> objects;
            unsigned int count;
        };

        struct ContainerChanges
        {
            std::vector<ContainerItem> items;
            unsigned int count;
        };

        RakNet::RakNetGUID guid;
        ObjectChanges objectChanges;
        ContainerChanges containerChanges;

        ESM::Cell cell;
    };
}

#endif //OPENMW_WORLDEVENT_HPP

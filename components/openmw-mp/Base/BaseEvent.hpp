#ifndef OPENMW_BASEEVENT_HPP
#define OPENMW_BASEEVENT_HPP

#include <components/esm/loadcell.hpp>
#include <components/openmw-mp/Base/BaseStructs.hpp>
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

    struct WorldObject
    {
        std::string refId;
        int refNumIndex;
        int mpNum;
        int count;
        int charge;
        int goldValue;

        ESM::Position position;

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

        bool isDisarmed;

        Target master;
        bool hasMaster;

        std::vector<ContainerItem> containerItems;
        unsigned int containerItemCount;
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
        
        std::vector<WorldObject> worldObjects;
        unsigned int worldObjectCount;

        ESM::Cell cell;

        unsigned char action; // 0 - Clear and set in entirety, 1 - Add item, 2 - Remove item, 3 - Request items

        bool isValid;
    };
}

#endif //OPENMW_BASEEVENT_HPP

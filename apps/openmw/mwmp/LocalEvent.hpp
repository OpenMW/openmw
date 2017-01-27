#ifndef OPENMW_LOCALEVENT_HPP
#define OPENMW_LOCALEVENT_HPP

#include <components/openmw-mp/Base/WorldEvent.hpp>
#include "../mwworld/cellstore.hpp"
#include <RakNetTypes.h>

namespace mwmp
{
    class Networking;
    class LocalEvent : public WorldEvent
    {
    public:

        LocalEvent(RakNet::RakNetGUID guid);
        virtual ~LocalEvent();

        void addCellRef(MWWorld::CellRef worldCellRef);
        void addRefId(std::string refId);

    private:
        Networking *getNetworking();

    };
}

#endif //OPENMW_LOCALEVENT_HPP

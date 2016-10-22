#ifndef OPENMW_WORLDEVENT_HPP
#define OPENMW_WORLDEVENT_HPP

#include <components/esm/cellid.hpp>
#include <components/esm/cellref.hpp>
#include <RakNetTypes.h>

namespace mwmp
{
    class WorldEvent
    {
    public:

        WorldEvent(RakNet::RakNetGUID guid) : guid(guid)
        {

        }

        WorldEvent()
        {

        }

        RakNet::RakNetGUID guid;

        ESM::CellId cellId;
        ESM::CellRef cellRef;
    };
}

#endif //OPENMW_WORLDEVENT_HPP

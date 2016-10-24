#ifndef OPENMW_WORLDEVENT_HPP
#define OPENMW_WORLDEVENT_HPP

#include <components/esm/loadcell.hpp>
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

        ESM::Cell cell;
        ESM::CellRef cellRef;
        int lockLevel;

        std::string video;
        bool allowSkipping;
    };
}

#endif //OPENMW_WORLDEVENT_HPP

#ifndef OPENMW_WORLDEVENT_HPP
#define OPENMW_WORLDEVENT_HPP

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

        virtual ESM::CellRef *CellRef()
        {
            return &ref;
        }

        RakNet::RakNetGUID guid;

    protected:

        ESM::CellRef ref;

    };
}

#endif //OPENMW_WORLDEVENT_HPP

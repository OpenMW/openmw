#ifndef OPENMW_WORLDEVENT_HPP
#define OPENMW_WORLDEVENT_HPP

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
    };
}

#endif //OPENMW_WORLDEVENT_HPP

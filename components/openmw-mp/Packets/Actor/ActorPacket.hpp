#ifndef OPENMW_ACTORPACKET_HPP
#define OPENMW_ACTORPACKET_HPP

#include <string>
#include <RakNetTypes.h>
#include <BitStream.h>
#include <PacketPriority.h>
#include <components/openmw-mp/Base/BaseEvent.hpp>

#include <components/openmw-mp/Packets/BasePacket.hpp>


namespace mwmp
{
    class ActorPacket : public BasePacket
    {
    public:
        ActorPacket(RakNet::RakPeerInterface *peer);

        ~ActorPacket();

        void setEvent(BaseEvent *event);

    protected:
        BaseEvent *event;

    };
}

#endif //OPENMW_ACTORPACKET_HPP

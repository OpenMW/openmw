#ifndef OPENMW_ACTORPACKETCONTROLLER_HPP
#define OPENMW_ACTORPACKETCONTROLLER_HPP


#include <RakPeerInterface.h>
#include "../Packets/Actor/ActorPacket.hpp"
#include <unordered_map>
#include <memory>

namespace mwmp
{
    class ActorPacketController
    {
    public:
        ActorPacketController(RakNet::RakPeerInterface *peer);
        ActorPacket *GetPacket(RakNet::MessageID id);
        void SetStream(RakNet::BitStream *inStream, RakNet::BitStream *outStream);

        bool ContainsPacket(RakNet::MessageID id);

        typedef std::unordered_map<unsigned char, std::unique_ptr<ActorPacket> > packets_t;
    private:
        packets_t packets;
    };
}

#endif //OPENMW_ACTORPACKETCONTROLLER_HPP

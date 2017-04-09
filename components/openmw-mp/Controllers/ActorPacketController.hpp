#ifndef OPENMW_ACTORPACKETCONTROLLER_HPP
#define OPENMW_ACTORPACKETCONTROLLER_HPP


#include <RakPeerInterface.h>
#include "../Packets/Actor/ActorPacket.hpp"
#include <map>
#include <boost/shared_ptr.hpp>

namespace mwmp
{
    class ActorPacketController
    {
    public:
        ActorPacketController(RakNet::RakPeerInterface *peer);
        ActorPacket *GetPacket(RakNet::MessageID id);
        void SetStream(RakNet::BitStream *inStream, RakNet::BitStream *outStream);

        bool ContainsPacket(RakNet::MessageID id);

        typedef std::map<unsigned char, boost::shared_ptr<ActorPacket> > packets_t;
    private:
        packets_t packets;
    };
}

#endif //OPENMW_ACTORPACKETCONTROLLER_HPP

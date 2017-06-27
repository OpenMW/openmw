#ifndef OPENMW_WORLDPACKETCONTROLLER_HPP
#define OPENMW_WORLDPACKETCONTROLLER_HPP


#include <RakPeerInterface.h>
#include "../Packets/World/WorldPacket.hpp"
#include <unordered_map>
#include <memory>

namespace mwmp
{
    class WorldPacketController
    {
    public:
        WorldPacketController(RakNet::RakPeerInterface *peer);
        WorldPacket *GetPacket(RakNet::MessageID id);
        void SetStream(RakNet::BitStream *inStream, RakNet::BitStream *outStream);

        bool ContainsPacket(RakNet::MessageID id);

        typedef std::unordered_map<unsigned char, std::unique_ptr<WorldPacket> > packets_t;
    private:
        packets_t packets;
    };
}

#endif //OPENMW_WORLDPACKETCONTROLLER_HPP

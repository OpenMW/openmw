#ifndef OPENMW_PLAYERPACKETCONTROLLER_HPP
#define OPENMW_PLAYERPACKETCONTROLLER_HPP


#include <RakPeerInterface.h>
#include "../Packets/Player/PlayerPacket.hpp"
#include <unordered_map>
#include <memory>

namespace mwmp
{
    class PlayerPacketController
    {
    public:
        PlayerPacketController(RakNet::RakPeerInterface *peer);
        PlayerPacket *GetPacket(RakNet::MessageID id);
        void SetStream(RakNet::BitStream *inStream, RakNet::BitStream *outStream);

        bool ContainsPacket(RakNet::MessageID id);

        typedef std::unordered_map<unsigned char, std::unique_ptr<PlayerPacket> > packets_t;
    private:
        packets_t packets;
    };
}

#endif //OPENMW_PLAYERPACKETCONTROLLER_HPP

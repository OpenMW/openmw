//
// Created by koncord on 05.03.17.
//

#ifndef OPENMW_PACKETPREINIT_HPP
#define OPENMW_PACKETPREINIT_HPP

#include <vector>
#include "BasePacket.hpp"


namespace mwmp
{
    class PacketPreInit : public BasePacket
    {
    public:
        typedef std::pair<std::string, unsigned int> PluginPair;
        typedef std::vector<PluginPair> PluginContainer;

        PacketPreInit(RakNet::RakPeerInterface *peer);

        virtual void Packet(RakNet::BitStream *bs, bool send, PluginContainer &checksums);
    };
}


#endif //OPENMW_PACKETPREINIT_HPP

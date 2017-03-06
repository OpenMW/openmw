//
// Created by koncord on 05.03.17.
//

#include <components/openmw-mp/NetworkMessages.hpp>
#include <boost/foreach.hpp>
#include "PacketPreInit.hpp"

mwmp::PacketPreInit::PacketPreInit(RakNet::RakPeerInterface *peer) : BasePacket(peer)
{
    packetID = ID_GAME_PREINIT;
}

void mwmp::PacketPreInit::Packet(RakNet::BitStream *bs, bool send)
{
    BasePacket::Packet(bs, send);

    unsigned int size = checksums->size();
    RW(size, send);
    if(send)
    {
        BOOST_FOREACH(PluginContainer::value_type & checksum, *checksums)
        {
            RW(checksum.first, true);
            RW(checksum.second, true);
        }
    }
    else
    {
        for(unsigned int i = 0; i < size; i++)
        {
            PluginPair checksum;
            RW(checksum.first, false);
            RW(checksum.second, false);
            checksums->push_back(checksum);
        }
    }
}

void mwmp::PacketPreInit::setChecksums(mwmp::PacketPreInit::PluginContainer *checksums)
{
    this->checksums = checksums;
}

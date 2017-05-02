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

    size_t size = checksums->size();
    RW(size, send);

    for (size_t i = 0; i < size; i++)
    {
        PluginPair ppair;
        if (send)
            ppair = (*checksums)[i];

        RW(ppair.first, send);

        size_t hashSize = ppair.second.size();
        RW(hashSize, send);
        for (size_t j = 0; j < hashSize; j++)
        {
            unsigned hash;
            if (send)
                hash = ppair.second[j];
            RW(hash, send);
            if (!send)
                ppair.second.push_back(hash);
        }
        if (!send)
            checksums->push_back(ppair);
    }
}

void mwmp::PacketPreInit::setChecksums(mwmp::PacketPreInit::PluginContainer *checksums)
{
    this->checksums = checksums;
}

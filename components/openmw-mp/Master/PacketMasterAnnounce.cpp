//
// Created by koncord on 22.04.17.
//

#include <components/openmw-mp/NetworkMessages.hpp>
#include <iostream>
#include "PacketMasterAnnounce.hpp"
#include "ProxyMasterPacket.hpp"

using namespace mwmp;
using namespace RakNet;
using namespace std;

PacketMasterAnnounce::PacketMasterAnnounce(RakNet::RakPeerInterface *peer) : BasePacket(peer)
{
    packetID = ID_MASTER_ANNOUNCE;
    orderChannel = CHANNEL_MASTER;
}

void PacketMasterAnnounce::Packet(BitStream *bs, bool send)
{
    this->bs = bs;
    if (send)
        bs->Write(packetID);

    RW(func, send);

    if (func == FUNCTION_ANNOUNCE)
        ProxyMasterPacket::addServer(this, *server, send);
}

void PacketMasterAnnounce::SetServer(QueryData *_server)
{
    server = _server;
}

void PacketMasterAnnounce::SetFunc(int _func)
{
    func = _func;
}

int PacketMasterAnnounce::GetFunc()
{
    return func;
}

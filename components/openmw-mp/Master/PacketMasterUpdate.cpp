//
// Created by koncord on 22.04.17.
//

#include <components/openmw-mp/NetworkMessages.hpp>
#include "PacketMasterUpdate.hpp"
#include "ProxyMasterPacket.hpp"

using namespace mwmp;
using namespace std;
using namespace RakNet;

PacketMasterUpdate::PacketMasterUpdate(RakNet::RakPeerInterface *peer) : BasePacket(peer)
{
    packetID = ID_MASTER_UPDATE;
    orderChannel = CHANNEL_MASTER;
}

void PacketMasterUpdate::Packet(RakNet::BitStream *bs, bool send)
{
    this->bs = bs;
    if (send)
        bs->Write(packetID);

    string addr = server->first.ToString(false);
    unsigned short port = server->first.GetPort();

    RW(addr, send);
    RW(port, send);

    if(!send)
        server->first = SystemAddress(addr.c_str(), port);

    ProxyMasterPacket::addServer(this, server->second, send);

}

void PacketMasterUpdate::SetServer(std::pair<RakNet::SystemAddress, QueryData> *serverPair)
{
    server = serverPair;
}

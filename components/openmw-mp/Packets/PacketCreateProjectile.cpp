//
// Created by koncord on 13.01.16.
//

#include <components/openmw-mp/NetworkMessages.hpp>
#include "PacketCreateProjectile.hpp"

mwmp::PacketCreateProjectile::PacketCreateProjectile(RakNet::RakPeerInterface *peer)
{
    packetID = ID_GAME_CREATE_PROJECTILE;
}

void mwmp::PacketCreateProjectile::Packet(RakNet::BitStream *bs, mwmp::BasePlayer *player, bool send)
{
    BasePacket::Packet(bs, player, send);


}

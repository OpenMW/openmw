#include <components/openmw-mp/NetworkMessages.hpp>
#include "PacketConsoleCommand.hpp"

using namespace mwmp;

PacketConsoleCommand::PacketConsoleCommand(RakNet::RakPeerInterface *peer) : WorldPacket(peer)
{
    packetID = ID_CONSOLE_COMMAND;
}

void PacketConsoleCommand::Object(WorldObject &worldObject, bool send)
{
    WorldPacket::Object(worldObject, send);
}

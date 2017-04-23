#include <components/openmw-mp/NetworkMessages.hpp>
#include <PacketPriority.h>
#include <RakPeer.h>
#include "BasePacket.hpp"

using namespace mwmp;

BasePacket::BasePacket(RakNet::RakPeerInterface *peer)
{
    packetID = 0;
    priority = HIGH_PRIORITY;
    reliability = RELIABLE_ORDERED;
    orderChannel = CHANNEL_SYSTEM;
    this->peer = peer;
}

BasePacket::~BasePacket()
{

}

void BasePacket::Packet(RakNet::BitStream *bs, bool send)
{
    this->bs = bs;

    if (send)
    {
        bs->Write(packetID);
        bs->Write(guid);
    }
}

void BasePacket::SetReadStream(RakNet::BitStream *bitStream)
{
    bsRead = bitStream;
}

void BasePacket::SetSendStream(RakNet::BitStream *bitStream)
{
    bsSend = bitStream;
}

void BasePacket::SetStreams(RakNet::BitStream *inStream, RakNet::BitStream *outStream)
{
    if (inStream != 0)
        bsRead = inStream;
    if (outStream != 0)
        bsSend = outStream;
}

void BasePacket::RequestData(RakNet::RakNetGUID guid)
{
    bsSend->ResetWritePointer();
    bsSend->Write(packetID);
    bsSend->Write(guid);
    peer->Send(bsSend, HIGH_PRIORITY, RELIABLE_ORDERED, orderChannel, guid, false);
}

void BasePacket::Send(RakNet::AddressOrGUID destination)
{
    bsSend->ResetWritePointer();
    Packet(bsSend, true);
    peer->Send(bsSend, priority, reliability, orderChannel, destination, false);
}

void BasePacket::Send(bool toOther)
{
    bsSend->ResetWritePointer();
    Packet(bsSend, true);
    peer->Send(bsSend, priority, reliability, orderChannel, guid, toOther);
}

void BasePacket::Read()
{
    Packet(bsRead, false);
}

void BasePacket::setGUID(RakNet::RakNetGUID guid)
{
    this->guid = guid;
}

RakNet::RakNetGUID BasePacket::getGUID()
{
    return guid;
}

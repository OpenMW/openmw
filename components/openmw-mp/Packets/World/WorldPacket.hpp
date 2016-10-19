#ifndef OPENMW_WORLDPACKET_HPP
#define OPENMW_WORLDPACKET_HPP

#include <string>
#include <RakNetTypes.h>
#include <BitStream.h>
#include <PacketPriority.h>
#include <components/openmw-mp/Base/BasePlayer.hpp>

#include <components/openmw-mp/Packets/BasePacket.hpp>


namespace mwmp
{
    class WorldPacket : public BasePacket
    {
    public:
        WorldPacket(RakNet::RakPeerInterface *peer);

        ~WorldPacket();

        virtual void Packet(RakNet::BitStream *bs, BasePlayer *player, bool send);

        virtual void Send(BasePlayer *player, bool toOtherPlayers = true);
        virtual void Send(BasePlayer *player, RakNet::AddressOrGUID destination);
        virtual void Read(BasePlayer *player);

        virtual void RequestData(RakNet::RakNetGUID player);

        static size_t headerSize()
        {
            return (size_t)(1 + RakNet::RakNetGUID::size()); // packetID + RakNetGUID (uint64_t)
        }

        unsigned char GetPacketID()
        {
            return packetID;
        }

    protected:
        BasePlayer *player;

    };
}

#endif //OPENMW_WORLDPACKET_HPP

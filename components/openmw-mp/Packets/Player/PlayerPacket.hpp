#ifndef OPENMW_PLAYERPACKET_HPP
#define OPENMW_PLAYERPACKET_HPP

#include <string>
#include <RakNetTypes.h>
#include <BitStream.h>
#include <PacketPriority.h>
#include <components/openmw-mp/Base/BasePlayer.hpp>

#include <components/openmw-mp/Packets/BasePacket.hpp>


namespace mwmp
{
    class PlayerPacket : public BasePacket
    {
    public:
        PlayerPacket(RakNet::RakPeerInterface *peer);

        ~PlayerPacket();

        virtual void Packet(RakNet::BitStream *bs, BasePlayer *player, bool send);

        virtual void Send(BasePlayer *player, bool toOtherPlayers = true);
        virtual void Send(BasePlayer *player, RakNet::AddressOrGUID destination);
        virtual void Read(BasePlayer *player);

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

#endif //OPENMW_PLAYERPACKET_HPP

#include <components/openmw-mp/NetworkMessages.hpp>
#include "PacketPlayerTopic.hpp"

using namespace std;
using namespace mwmp;

PacketPlayerTopic::PacketPlayerTopic(RakNet::RakPeerInterface *peer) : PlayerPacket(peer)
{
    packetID = ID_PLAYER_TOPIC;
}

void PacketPlayerTopic::Packet(RakNet::BitStream *bs, bool send)
{
    PlayerPacket::Packet(bs, send);

    if (send)
        player->topicChanges.count = (unsigned int)(player->topicChanges.topics.size());
    else
        player->topicChanges.topics.clear();

    RW(player->topicChanges.count, send);

    for (unsigned int i = 0; i < player->topicChanges.count; i++)
    {
        Topic topic;

        if (send)
            topic = player->topicChanges.topics.at(i);

        RW(topic.topicId, send, 1);

        if (!send)
            player->topicChanges.topics.push_back(topic);
    }
}

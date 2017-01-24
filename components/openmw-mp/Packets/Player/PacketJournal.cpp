#include <components/openmw-mp/NetworkMessages.hpp>
#include "PacketJournal.hpp"

using namespace std;
using namespace mwmp;

PacketJournal::PacketJournal(RakNet::RakPeerInterface *peer) : PlayerPacket(peer)
{
    packetID = ID_GAME_JOURNAL;
}

void PacketJournal::Packet(RakNet::BitStream *bs, BasePlayer *player, bool send)
{
    PlayerPacket::Packet(bs, player, send);

    if (!send)
        player->journalChanges.journalItems.clear();
    else
        player->journalChanges.count = (unsigned int)(player->journalChanges.journalItems.size());

    RW(player->journalChanges.count, send);

    for (unsigned int i = 0; i < player->journalChanges.count; i++)
    {
        JournalItem journalItem;

        if (send)
            journalItem = player->journalChanges.journalItems[i];

        RW(journalItem.type, send);
        RW(journalItem.quest, send);
        RW(journalItem.index, send);

        if (journalItem.type == JournalItem::ENTRY)
        {
            RW(journalItem.actorCellRef.mRefID, send);
            RW(journalItem.actorCellRef.mRefNum.mIndex, send);
            RW(journalItem.actorCell.mData.mFlags, send);
            RW(journalItem.actorCell.mData.mX, send);
            RW(journalItem.actorCell.mData.mY, send);
            RW(journalItem.actorCell.mName, send);
        }

        if (!send)
            player->journalChanges.journalItems.push_back(journalItem);
    }
}

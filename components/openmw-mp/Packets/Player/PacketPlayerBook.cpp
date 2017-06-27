#include <components/openmw-mp/NetworkMessages.hpp>
#include "PacketPlayerBook.hpp"

using namespace std;
using namespace mwmp;

PacketPlayerBook::PacketPlayerBook(RakNet::RakPeerInterface *peer) : PlayerPacket(peer)
{
    packetID = ID_PLAYER_BOOK;
}

void PacketPlayerBook::Packet(RakNet::BitStream *bs, bool send)
{
    PlayerPacket::Packet(bs, send);

    if (send)
        player->bookChanges.count = (unsigned int)(player->bookChanges.books.size());
    else
        player->bookChanges.books.clear();

    RW(player->bookChanges.count, send);

    for (unsigned int i = 0; i < player->bookChanges.count; i++)
    {
        Book book;

        if (send)
            book = player->bookChanges.books.at(i);

        RW(book.bookId, send, 1);

        if (!send)
            player->bookChanges.books.push_back(book);
    }
}

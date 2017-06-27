#include "Books.hpp"
#include <apps/openmw-mp/Script/ScriptFunctions.hpp>
#include <components/openmw-mp/NetworkMessages.hpp>
#include <apps/openmw-mp/Networking.hpp>

using namespace mwmp;

unsigned int BookFunctions::GetBookChangesSize(unsigned short pid) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, 0);

    return player->bookChanges.count;
}

void BookFunctions::AddBook(unsigned short pid, const char* bookId) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, );

    mwmp::Book book;
    book.bookId = bookId;

    player->bookChangesBuffer.books.push_back(book);
}

const char *BookFunctions::GetBookId(unsigned short pid, unsigned int i) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, "");

    if (i >= player->bookChanges.count)
        return "invalid";

    return player->bookChanges.books.at(i).bookId.c_str();
}

void BookFunctions::SendBookChanges(unsigned short pid) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, );

    std::swap(player->bookChanges, player->bookChangesBuffer);
    mwmp::Networking::get().getPlayerPacketController()->GetPacket(ID_PLAYER_BOOK)->setPlayer(player);
    mwmp::Networking::get().getPlayerPacketController()->GetPacket(ID_PLAYER_BOOK)->Send(false);
    player->bookChanges = std::move(player->bookChangesBuffer);
    player->bookChangesBuffer.books.clear();
}

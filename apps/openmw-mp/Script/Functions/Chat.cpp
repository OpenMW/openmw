//
// Created by koncord on 29.04.16.
//

#include <apps/openmw-mp/Script/ScriptFunctions.hpp>
#include <apps/openmw-mp/Networking.hpp>
#include <components/openmw-mp/NetworkMessages.hpp>

void ScriptFunctions::SendMessage(unsigned short pid, const char *message, bool broadcast) noexcept
{
    Player *player;
    GET_PLAYER(pid, player,);

    player->chatMessage = message;

    LOG_MESSAGE_SIMPLE(Log::LOG_VERBOSE, "System: %s", message);

    mwmp::Networking::get().getPlayerPacketController()->GetPacket(ID_CHAT_MESSAGE)->setPlayer(player);

    mwmp::Networking::get().getPlayerPacketController()->GetPacket(ID_CHAT_MESSAGE)->Send(false);
    if (broadcast)
        mwmp::Networking::get().getPlayerPacketController()->GetPacket(ID_CHAT_MESSAGE)->Send(true);
}

void ScriptFunctions::CleanChatByPid(unsigned short pid)
{
    Player *player;
    GET_PLAYER(pid, player,);

    player->chatMessage.clear();

    mwmp::Networking::get().getPlayerPacketController()->GetPacket(ID_CHAT_MESSAGE)->setPlayer(player);

    mwmp::Networking::get().getPlayerPacketController()->GetPacket(ID_CHAT_MESSAGE)->Send(false);
}

void ScriptFunctions::CleanChat()
{
    for (auto player : *Players::getPlayers())
    {
        player.second->chatMessage.clear();
        mwmp::Networking::get().getPlayerPacketController()->GetPacket(ID_CHAT_MESSAGE)->setPlayer(player.second);

        mwmp::Networking::get().getPlayerPacketController()->GetPacket(ID_CHAT_MESSAGE)->Send(false);
    }
}

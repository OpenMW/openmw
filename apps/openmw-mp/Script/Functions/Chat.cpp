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

    *player->ChatMessage() = message;

    DEBUG_PRINTF("System: %s", message);

    mwmp::Networking::Get().GetController()->GetPacket(ID_CHAT_MESSAGE)->Send(player, false);
    if (broadcast)
        mwmp::Networking::Get().GetController()->GetPacket(ID_CHAT_MESSAGE)->Send(player, true);
}

void ScriptFunctions::CleanChat(unsigned short pid)
{

}

void ScriptFunctions::CleanChat()
{

}

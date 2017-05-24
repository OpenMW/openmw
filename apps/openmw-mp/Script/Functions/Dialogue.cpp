#include "Dialogue.hpp"
#include <apps/openmw-mp/Script/ScriptFunctions.hpp>
#include <components/openmw-mp/NetworkMessages.hpp>
#include <apps/openmw-mp/Networking.hpp>
#include <components/misc/stringops.hpp>

using namespace mwmp;

unsigned int DialogueFunctions::GetTopicChangesSize(unsigned short pid) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, 0);

    return player->topicChanges.count;
}

void DialogueFunctions::AddTopic(unsigned short pid, const char* topicId) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, );

    mwmp::Topic topic;
    topic.topicId = topicId;

    player->topicChangesBuffer.topics.push_back(topic);
}

const char *DialogueFunctions::GetTopicId(unsigned short pid, unsigned int i) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, "");

    if (i >= player->topicChanges.count)
        return "invalid";

    return player->topicChanges.topics.at(i).topicId.c_str();
}

void DialogueFunctions::SendTopicChanges(unsigned short pid) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, );

    std::swap(player->topicChanges, player->topicChangesBuffer);
    mwmp::Networking::get().getPlayerPacketController()->GetPacket(ID_PLAYER_TOPIC)->setPlayer(player);
    mwmp::Networking::get().getPlayerPacketController()->GetPacket(ID_PLAYER_TOPIC)->Send(false);
    player->topicChanges = std::move(player->topicChangesBuffer);
    player->topicChangesBuffer.topics.clear();
}

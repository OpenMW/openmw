#include "Dialogue.hpp"
#include <apps/openmw-mp/Script/ScriptFunctions.hpp>
#include <components/openmw-mp/NetworkMessages.hpp>
#include <apps/openmw-mp/Networking.hpp>

using namespace mwmp;

void DialogueFunctions::InitializeTopicChanges(unsigned short pid) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, );

    player->topicChanges.topics.clear();
}

void DialogueFunctions::InitializeKillChanges(unsigned short pid) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, );

    player->killChanges.kills.clear();
}

unsigned int DialogueFunctions::GetTopicChangesSize(unsigned short pid) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, 0);

    return player->topicChanges.count;
}

unsigned int DialogueFunctions::GetKillChangesSize(unsigned short pid) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, 0);

    return player->killChanges.count;
}

void DialogueFunctions::AddTopic(unsigned short pid, const char* topicId) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, );

    mwmp::Topic topic;
    topic.topicId = topicId;

    player->topicChanges.topics.push_back(topic);
}

void DialogueFunctions::AddKill(unsigned short pid, const char* refId, int number) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, );

    mwmp::Kill kill;
    kill.refId = refId;
    kill.number = number;

    player->killChanges.kills.push_back(kill);
}

const char *DialogueFunctions::GetTopicId(unsigned short pid, unsigned int i) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, "");

    if (i >= player->topicChanges.count)
        return "invalid";

    return player->topicChanges.topics.at(i).topicId.c_str();
}

const char *DialogueFunctions::GetKillRefId(unsigned short pid, unsigned int i) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, "");

    if (i >= player->killChanges.count)
        return "invalid";

    return player->killChanges.kills.at(i).refId.c_str();
}

int DialogueFunctions::GetKillNumber(unsigned short pid, unsigned int i) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, 0);

    return player->killChanges.kills.at(i).number;
}

void DialogueFunctions::SendTopicChanges(unsigned short pid, bool toOthers) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, );

    mwmp::Networking::get().getPlayerPacketController()->GetPacket(ID_PLAYER_TOPIC)->setPlayer(player);
    mwmp::Networking::get().getPlayerPacketController()->GetPacket(ID_PLAYER_TOPIC)->Send(toOthers);
}

void DialogueFunctions::SendKillChanges(unsigned short pid, bool toOthers) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, );

    mwmp::Networking::get().getPlayerPacketController()->GetPacket(ID_PLAYER_KILL_COUNT)->setPlayer(player);
    mwmp::Networking::get().getPlayerPacketController()->GetPacket(ID_PLAYER_KILL_COUNT)->Send(toOthers);
}

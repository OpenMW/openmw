#include "Quests.hpp"
#include <apps/openmw-mp/Script/ScriptFunctions.hpp>
#include <components/openmw-mp/NetworkMessages.hpp>
#include <apps/openmw-mp/Networking.hpp>
#include <components/misc/stringops.hpp>

using namespace mwmp;

void QuestFunctions::InitializeJournalChanges(unsigned short pid) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, );

    player->journalChanges.journalItems.clear();
}

unsigned int QuestFunctions::GetJournalChangesSize(unsigned short pid) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, 0);

    return player->journalChanges.count;
}

void QuestFunctions::AddJournalEntry(unsigned short pid, const char* quest, unsigned int index, const char* actorRefId) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, );

    mwmp::JournalItem journalItem;
    journalItem.type = JournalItem::ENTRY;
    journalItem.quest = quest;
    journalItem.index = index;
    journalItem.actorRefId = actorRefId;

    player->journalChanges.journalItems.push_back(journalItem);
}

void QuestFunctions::AddJournalIndex(unsigned short pid, const char* quest, unsigned int index) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, );

    mwmp::JournalItem journalItem;
    journalItem.type = JournalItem::INDEX;
    journalItem.quest = quest;
    journalItem.index = index;

    player->journalChanges.journalItems.push_back(journalItem);
}

const char *QuestFunctions::GetJournalItemQuest(unsigned short pid, unsigned int i) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, "");

    if (i >= player->journalChanges.count)
        return "invalid";

    return player->journalChanges.journalItems.at(i).quest.c_str();
}

int QuestFunctions::GetJournalItemIndex(unsigned short pid, unsigned int i) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, 0);

    return player->journalChanges.journalItems.at(i).index;
}

int QuestFunctions::GetJournalItemType(unsigned short pid, unsigned int i) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, 0);

    return player->journalChanges.journalItems.at(i).type;
}

const char *QuestFunctions::GetJournalItemActorRefId(unsigned short pid, unsigned int i) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, 0);

    return player->journalChanges.journalItems.at(i).actorRefId.c_str();
}

void QuestFunctions::SendJournalChanges(unsigned short pid, bool toOthers) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, );

    mwmp::Networking::get().getPlayerPacketController()->GetPacket(ID_PLAYER_JOURNAL)->setPlayer(player);
    mwmp::Networking::get().getPlayerPacketController()->GetPacket(ID_PLAYER_JOURNAL)->Send(toOthers);
}

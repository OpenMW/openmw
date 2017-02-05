#include "Quests.hpp"
#include <apps/openmw-mp/Script/ScriptFunctions.hpp>
#include <components/openmw-mp/NetworkMessages.hpp>
#include <apps/openmw-mp/Networking.hpp>
#include <components/misc/stringops.hpp>

using namespace mwmp;

unsigned int QuestFunctions::GetJournalChangesSize(unsigned short pid) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, 0);

    return player->journalChanges.count;
}

void QuestFunctions::AddJournalEntry(unsigned short pid, const char* quest, unsigned int index) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, );

    mwmp::JournalItem journalItem;
    journalItem.type = JournalItem::ENTRY;
    journalItem.quest = quest;
    journalItem.index = index;

    player->journalChangesBuffer.journalItems.push_back(journalItem);
}

void QuestFunctions::AddJournalIndex(unsigned short pid, const char* quest, unsigned int index) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, );

    mwmp::JournalItem journalItem;
    journalItem.type = JournalItem::INDEX;
    journalItem.quest = quest;
    journalItem.index = index;

    player->journalChangesBuffer.journalItems.push_back(journalItem);
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

void QuestFunctions::SendJournalChanges(unsigned short pid) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, );

    std::swap(player->journalChanges, player->journalChangesBuffer);
    mwmp::Networking::get().getPlayerController()->GetPacket(ID_PLAYER_JOURNAL)->Send(player, false);
    player->journalChanges = std::move(player->journalChangesBuffer);
    player->journalChangesBuffer.journalItems.clear();
}

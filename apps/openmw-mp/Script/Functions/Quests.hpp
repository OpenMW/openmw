#ifndef OPENMW_QUESTS_HPP
#define OPENMW_QUESTS_HPP

#define QUESTAPI \
    {"GetJournalChangesSize",   QuestFunctions::GetJournalChangesSize},\
    \
    {"AddJournalEntry",         QuestFunctions::AddJournalEntry},\
    {"AddJournalIndex",         QuestFunctions::AddJournalIndex},\
    \
    {"GetJournalItemQuest",     QuestFunctions::GetJournalItemQuest},\
    {"GetJournalItemIndex",     QuestFunctions::GetJournalItemIndex},\
    {"GetJournalItemType",      QuestFunctions::GetJournalItemType},\
    \
    {"SendJournalChanges",      QuestFunctions::SendJournalChanges}

class QuestFunctions
{
public:

    static unsigned int GetJournalChangesSize(unsigned short pid) noexcept;

    static void AddJournalEntry(unsigned short pid, const char* quest, unsigned int index) noexcept;
    static void AddJournalIndex(unsigned short pid, const char* quest, unsigned int index) noexcept;

    static const char *GetJournalItemQuest(unsigned short pid, unsigned int i) noexcept;
    static int GetJournalItemIndex(unsigned short pid, unsigned int i) noexcept;
    static int GetJournalItemType(unsigned short pid, unsigned int i) noexcept;

    static void SendJournalChanges(unsigned short pid) noexcept;
private:

};

#endif //OPENMW_QUESTS_HPP

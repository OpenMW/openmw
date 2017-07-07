#ifndef OPENMW_QUESTAPI_HPP
#define OPENMW_QUESTAPI_HPP

#define QUESTAPI \
    {"InitializeJournalChanges",  QuestFunctions::InitializeJournalChanges},\
    \
    {"GetJournalChangesSize",     QuestFunctions::GetJournalChangesSize},\
    \
    {"AddJournalEntry",           QuestFunctions::AddJournalEntry},\
    {"AddJournalIndex",           QuestFunctions::AddJournalIndex},\
    \
    {"GetJournalItemQuest",       QuestFunctions::GetJournalItemQuest},\
    {"GetJournalItemIndex",       QuestFunctions::GetJournalItemIndex},\
    {"GetJournalItemType",        QuestFunctions::GetJournalItemType},\
    {"GetJournalItemActorRefId",  QuestFunctions::GetJournalItemActorRefId},\
    \
    {"SendJournalChanges",        QuestFunctions::SendJournalChanges}

class QuestFunctions
{
public:
    static void InitializeJournalChanges(unsigned short pid) noexcept;

    static unsigned int GetJournalChangesSize(unsigned short pid) noexcept;

    static void AddJournalEntry(unsigned short pid, const char* quest, unsigned int index, const char* actorRefId) noexcept;
    static void AddJournalIndex(unsigned short pid, const char* quest, unsigned int index) noexcept;

    static const char *GetJournalItemQuest(unsigned short pid, unsigned int i) noexcept;
    static int GetJournalItemIndex(unsigned short pid, unsigned int i) noexcept;
    static int GetJournalItemType(unsigned short pid, unsigned int i) noexcept;
    static const char *GetJournalItemActorRefId(unsigned short pid, unsigned int i) noexcept;

    static void SendJournalChanges(unsigned short pid, bool toOthers = false) noexcept;
private:

};

#endif //OPENMW_QUESTAPI_HPP

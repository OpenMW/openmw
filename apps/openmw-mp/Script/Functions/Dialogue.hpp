#ifndef OPENMW_DIALOGUEAPI_HPP
#define OPENMW_DIALOGUEAPI_HPP

#define DIALOGUEAPI \
    {"InitializeTopicChanges",  DialogueFunctions::InitializeTopicChanges},\
    {"InitializeKillChanges",   DialogueFunctions::InitializeKillChanges},\
    \
    {"GetTopicChangesSize",     DialogueFunctions::GetTopicChangesSize},\
    {"GetKillChangesSize",      DialogueFunctions::GetKillChangesSize},\
    \
    {"AddTopic",                DialogueFunctions::AddTopic},\
    {"AddKill",                 DialogueFunctions::AddKill},\
    \
    {"GetTopicId",              DialogueFunctions::GetTopicId},\
    {"GetKillRefId",            DialogueFunctions::GetKillRefId},\
    {"GetKillNumber",           DialogueFunctions::GetKillNumber},\
    \
    {"SendTopicChanges",        DialogueFunctions::SendTopicChanges},\
    {"SendKillChanges",         DialogueFunctions::SendKillChanges}

class DialogueFunctions
{
public:

    static void InitializeTopicChanges(unsigned short pid) noexcept;
    static void InitializeKillChanges(unsigned short pid) noexcept;

    static unsigned int GetTopicChangesSize(unsigned short pid) noexcept;
    static unsigned int GetKillChangesSize(unsigned short pid) noexcept;

    static void AddTopic(unsigned short pid, const char* topicId) noexcept;
    static void AddKill(unsigned short pid, const char* refId, int number) noexcept;

    static const char *GetTopicId(unsigned short pid, unsigned int i) noexcept;
    static const char *GetKillRefId(unsigned short pid, unsigned int i) noexcept;
    static int GetKillNumber(unsigned short pid, unsigned int i) noexcept;

    static void SendTopicChanges(unsigned short pid, bool toOthers = false) noexcept;
    static void SendKillChanges(unsigned short pid, bool toOthers = false) noexcept;
private:

};

#endif //OPENMW_DIALOGUEAPI_HPP

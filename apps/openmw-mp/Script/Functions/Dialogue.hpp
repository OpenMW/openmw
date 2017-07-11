#ifndef OPENMW_DIALOGUEAPI_HPP
#define OPENMW_DIALOGUEAPI_HPP

#define DIALOGUEAPI \
    {"InitializeTopicChanges",  DialogueFunctions::InitializeTopicChanges},\
    \
    {"GetTopicChangesSize",     DialogueFunctions::GetTopicChangesSize},\
    \
    {"AddTopic",                DialogueFunctions::AddTopic},\
    \
    {"GetTopicId",              DialogueFunctions::GetTopicId},\
    \
    {"SendTopicChanges",        DialogueFunctions::SendTopicChanges}

class DialogueFunctions
{
public:

    static void InitializeTopicChanges(unsigned short pid) noexcept;

    static unsigned int GetTopicChangesSize(unsigned short pid) noexcept;

    static void AddTopic(unsigned short pid, const char* topicId) noexcept;

    static const char *GetTopicId(unsigned short pid, unsigned int i) noexcept;

    static void SendTopicChanges(unsigned short pid, bool toOthers = false) noexcept;
private:

};

#endif //OPENMW_DIALOGUEAPI_HPP

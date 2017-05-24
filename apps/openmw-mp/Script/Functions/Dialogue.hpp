#ifndef OPENMW_DIALOGUEAPI_HPP
#define OPENMW_DIALOGUEAPI_HPP

#define DIALOGUEAPI \
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

    static unsigned int GetTopicChangesSize(unsigned short pid) noexcept;

    static void AddTopic(unsigned short pid, const char* topicId) noexcept;

    static const char *GetTopicId(unsigned short pid, unsigned int i) noexcept;

    static void SendTopicChanges(unsigned short pid) noexcept;
private:

};

#endif //OPENMW_DIALOGUEAPI_HPP

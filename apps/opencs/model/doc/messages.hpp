#ifndef CSM_DOC_MESSAGES_H
#define CSM_DOC_MESSAGES_H

#include <string>
#include <vector>

#include <QMetaType>

#include "../world/universalid.hpp"

namespace CSMDoc
{
    struct Message
    {
        enum Severity
        {
            Severity_Info = 0,         // no problem
            Severity_Warning = 1,      // a potential problem, but we are probably fine
            Severity_Error = 2,        // an error; we are not fine
            Severity_SeriousError = 3, // an error so bad we can't even be sure if we are
                                       // reporting it correctly
            Severity_Default = 4
        };

        CSMWorld::UniversalId mId;
        std::string mMessage;
        std::string mHint;
        Severity mSeverity;

        Message();

        Message (const CSMWorld::UniversalId& id, const std::string& message,
            const std::string& hint, Severity severity);

        static std::string toString (Severity severity);
    };

    class Messages
    {
        public:

            typedef std::vector<Message> Collection;

            typedef Collection::const_iterator Iterator;

        private:

            Collection mMessages;
            Message::Severity mDefault;

        public:

            Messages (Message::Severity default_);

            void add (const CSMWorld::UniversalId& id, const std::string& message,
                const std::string& hint = "",
                Message::Severity severity = Message::Severity_Default);

            Iterator begin() const;

            Iterator end() const;
    };
}

Q_DECLARE_METATYPE (CSMDoc::Message)

#endif

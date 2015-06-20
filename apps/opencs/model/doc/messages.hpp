#ifndef CSM_DOC_MESSAGES_H
#define CSM_DOC_MESSAGES_H

#include <string>
#include <vector>

#include "../world/universalid.hpp"

namespace CSMDoc
{
    struct Message
    {
        CSMWorld::UniversalId mId;
        std::string mMessage;
        std::string mHint;

        Message (const CSMWorld::UniversalId& id, const std::string& message,
            const std::string& hint);
    };
            
    class Messages
    {
        public:

            // \deprecated Use CSMDoc::Message directly instead.
            typedef CSMDoc::Message Message;

            typedef std::vector<Message> Collection;

            typedef Collection::const_iterator Iterator;

        private:

            Collection mMessages;

        public:

            void add (const CSMWorld::UniversalId& id, const std::string& message,
                const std::string& hint = "");

            /// \deprecated Use add instead.
            void push_back (const std::pair<CSMWorld::UniversalId, std::string>& data);

            Iterator begin() const;

            Iterator end() const;
    };
}

#endif

#ifndef GAME_MWBASE_DIALOGUEMANAGER_H
#define GAME_MWBASE_DIALOGUEMANAGER_H

#include <list>
#include <map>
#include <string>
#include <string_view>
#include <vector>

#include <cstdint>

namespace Loading
{
    class Listener;
}

namespace ESM
{
    class ESMReader;
    class ESMWriter;
    class RefId;
}

namespace MWWorld
{
    class Ptr;
}

namespace MWBase
{
    /// \brief Interface for dialogue manager (implemented in MWDialogue)
    class DialogueManager
    {
        DialogueManager(const DialogueManager&);
        ///< not implemented

        DialogueManager& operator=(const DialogueManager&);
        ///< not implemented

    public:
        class ResponseCallback
        {
        public:
            virtual ~ResponseCallback() = default;
            virtual void addResponse(std::string_view title, std::string_view text) = 0;
        };

        DialogueManager() {}

        virtual void clear() = 0;

        virtual ~DialogueManager() {}

        virtual bool isInChoice() const = 0;

        virtual bool startDialogue(const MWWorld::Ptr& actor, ResponseCallback* callback) = 0;

        virtual bool inJournal(const ESM::RefId& topicId, const ESM::RefId& infoId) const = 0;

        virtual void addTopic(const ESM::RefId& topic) = 0;

        virtual void addChoice(std::string_view text, int choice) = 0;
        virtual const std::vector<std::pair<std::string, int>>& getChoices() const = 0;

        virtual bool isGoodbye() const = 0;

        virtual void goodbye() = 0;

        virtual bool say(const MWWorld::Ptr& actor, const ESM::RefId& topic) = 0;

        virtual void keywordSelected(std::string_view keyword, ResponseCallback* callback) = 0;
        virtual void goodbyeSelected() = 0;
        virtual void questionAnswered(int answer, ResponseCallback* callback) = 0;

        enum TopicType
        {
            Specific = 1,
            Exhausted = 2
        };

        enum ServiceType
        {
            Any = -1,
            Barter = 1,
            Repair = 2,
            Spells = 3,
            Training = 4,
            Travel = 5,
            Spellmaking = 6,
            Enchanting = 7
        };

        virtual std::list<std::string> getAvailableTopics() = 0;
        virtual int getTopicFlag(const ESM::RefId&) const = 0;

        virtual bool checkServiceRefused(ResponseCallback* callback, ServiceType service = ServiceType::Any) = 0;

        virtual void persuade(int type, ResponseCallback* callback) = 0;

        /// @note Controlled by an option, gets discarded when dialogue ends by default
        virtual void applyBarterDispositionChange(int delta) = 0;

        virtual int countSavedGameRecords() const = 0;

        virtual void write(ESM::ESMWriter& writer, Loading::Listener& progress) const = 0;

        virtual void readRecord(ESM::ESMReader& reader, uint32_t type) = 0;

        /// Changes faction1's opinion of faction2 by \a diff.
        virtual void modFactionReaction(const ESM::RefId& faction1, const ESM::RefId& faction2, int diff) = 0;

        /// Set faction1's opinion of faction2.
        virtual void setFactionReaction(const ESM::RefId& faction1, const ESM::RefId& faction2, int absolute) = 0;

        /// @return faction1's opinion of faction2
        virtual int getFactionReaction(const ESM::RefId& faction1, const ESM::RefId& faction2) const = 0;

        /// @return all faction's opinion overrides
        virtual const std::map<ESM::RefId, int>* getFactionReactionOverrides(const ESM::RefId& faction) const = 0;

        /// Removes the last added topic response for the given actor from the journal
        virtual void clearInfoActor(const MWWorld::Ptr& actor) const = 0;
    };
}

#endif

#ifndef GAME_MWDIALOG_DIALOGUEMANAGERIMP_H
#define GAME_MWDIALOG_DIALOGUEMANAGERIMP_H

#include "../mwbase/dialoguemanager.hpp"

#include <map>
#include <set>
#include <unordered_map>

#include <components/compiler/streamerrorhandler.hpp>
#include <components/translation/translation.hpp>
#include <components/misc/stringops.hpp>

#include "../mwworld/ptr.hpp"

#include "../mwscript/compilercontext.hpp"

namespace ESM
{
    struct Dialogue;
}

namespace MWDialogue
{
    class DialogueManager : public MWBase::DialogueManager
    {
            std::set<std::string, Misc::StringUtils::CiComp> mKnownTopics;// Those are the topics the player knows.

            // Modified faction reactions. <Faction1, <Faction2, Difference> >
            typedef std::map<std::string, std::map<std::string, int> > ModFactionReactionMap;
            ModFactionReactionMap mChangedFactionReaction;

            std::set<std::string, Misc::StringUtils::CiComp> mActorKnownTopics;
            std::unordered_map<std::string, int> mActorKnownTopicsFlag;

            Translation::Storage& mTranslationDataStorage;
            MWScript::CompilerContext mCompilerContext;
            Compiler::StreamErrorHandler mErrorHandler;

            MWWorld::Ptr mActor;
            bool mTalkedTo;

            int mChoice;
            std::string mLastTopic; // last topic ID, lowercase
            bool mIsInChoice;
            bool mGoodbye;

            std::vector<std::pair<std::string, int> > mChoices;

            float mTemporaryDispositionChange;
            float mPermanentDispositionChange;

            void parseText (const std::string& text);

            void updateActorKnownTopics();
            void updateGlobals();

            bool compile (const std::string& cmd, std::vector<Interpreter::Type_Code>& code, const MWWorld::Ptr& actor);
            void executeScript (const std::string& script, const MWWorld::Ptr& actor);

            void executeTopic (const std::string& topic, ResponseCallback* callback);

            const ESM::Dialogue* searchDialogue(const std::string& id);

        public:

            DialogueManager (const Compiler::Extensions& extensions, Translation::Storage& translationDataStorage);

            void clear() override;

            bool isInChoice() const override;

            bool startDialogue (const MWWorld::Ptr& actor, ResponseCallback* callback) override;

            std::list<std::string> getAvailableTopics() override;
            int getTopicFlag(const std::string& topicId) override;

            bool inJournal (const std::string& topicId, const std::string& infoId) override;

            void addTopic (const std::string& topic) override;

            void addChoice (const std::string& text,int choice) override;
            const std::vector<std::pair<std::string, int> >& getChoices() override;

            bool isGoodbye() override;

            void goodbye() override;

            bool checkServiceRefused (ResponseCallback* callback, ServiceType service = ServiceType::Any) override;

            void say(const MWWorld::Ptr &actor, const std::string &topic) override;

            //calbacks for the GUI
            void keywordSelected (const std::string& keyword, ResponseCallback* callback) override;
            void goodbyeSelected() override;
            void questionAnswered (int answer, ResponseCallback* callback) override;

            void persuade (int type, ResponseCallback* callback) override;
            int getTemporaryDispositionChange () const override;

            /// @note Controlled by an option, gets discarded when dialogue ends by default
            void applyBarterDispositionChange (int delta) override;

            int countSavedGameRecords() const override;

            void write (ESM::ESMWriter& writer, Loading::Listener& progress) const override;

            void readRecord (ESM::ESMReader& reader, uint32_t type) override;

            /// Changes faction1's opinion of faction2 by \a diff.
            void modFactionReaction (const std::string& faction1, const std::string& faction2, int diff) override;

            void setFactionReaction (const std::string& faction1, const std::string& faction2, int absolute) override;

            /// @return faction1's opinion of faction2
            int getFactionReaction (const std::string& faction1, const std::string& faction2) const override;

            /// Removes the last added topic response for the given actor from the journal
            void clearInfoActor (const MWWorld::Ptr& actor) const override;
    };
}

#endif

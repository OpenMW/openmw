#ifndef GAME_MWDIALOG_DIALOGUEMANAGERIMP_H
#define GAME_MWDIALOG_DIALOGUEMANAGERIMP_H

#include "../mwbase/dialoguemanager.hpp"

#include <map>
#include <set>

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

            virtual void clear();

            virtual bool isInChoice() const;

            virtual bool startDialogue (const MWWorld::Ptr& actor, ResponseCallback* callback);

            std::list<std::string> getAvailableTopics();

            virtual void addTopic (const std::string& topic);

            virtual void addChoice (const std::string& text,int choice);
            const std::vector<std::pair<std::string, int> >& getChoices();

            virtual bool isGoodbye();

            virtual void goodbye();

            virtual bool checkServiceRefused (ResponseCallback* callback);

            virtual void say(const MWWorld::Ptr &actor, const std::string &topic);

            //calbacks for the GUI
            virtual void keywordSelected (const std::string& keyword, ResponseCallback* callback);
            virtual void goodbyeSelected();
            virtual void questionAnswered (int answer, ResponseCallback* callback);

            virtual void persuade (int type, ResponseCallback* callback);
            virtual int getTemporaryDispositionChange () const;

            /// @note Controlled by an option, gets discarded when dialogue ends by default
            virtual void applyBarterDispositionChange (int delta);

            virtual int countSavedGameRecords() const;

            virtual void write (ESM::ESMWriter& writer, Loading::Listener& progress) const;

            virtual void readRecord (ESM::ESMReader& reader, uint32_t type);

            /// Changes faction1's opinion of faction2 by \a diff.
            virtual void modFactionReaction (const std::string& faction1, const std::string& faction2, int diff);

            virtual void setFactionReaction (const std::string& faction1, const std::string& faction2, int absolute);

            /// @return faction1's opinion of faction2
            virtual int getFactionReaction (const std::string& faction1, const std::string& faction2) const;

            /// Removes the last added topic response for the given actor from the journal
            virtual void clearInfoActor (const MWWorld::Ptr& actor) const;
    };
}

#endif

#ifndef GAME_MWDIALOG_DIALOGUEMANAGERIMP_H
#define GAME_MWDIALOG_DIALOGUEMANAGERIMP_H

#include "../mwbase/dialoguemanager.hpp"

#include <map>
#include <set>

#include <components/compiler/streamerrorhandler.hpp>
#include <components/translation/translation.hpp>

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
            std::map<std::string, ESM::Dialogue> mDialogueMap;
            std::set<std::string> mKnownTopics;// Those are the topics the player knows.

            // Modified faction reactions. <Faction1, <Faction2, Difference> >
            typedef std::map<std::string, std::map<std::string, int> > ModFactionReactionMap;
            ModFactionReactionMap mChangedFactionReaction;

            std::set<std::string> mActorKnownTopics;

            Translation::Storage& mTranslationDataStorage;
            MWScript::CompilerContext mCompilerContext;
            std::ostream mErrorStream;
            Compiler::StreamErrorHandler mErrorHandler;

            MWWorld::Ptr mActor;
            bool mTalkedTo;

            int mChoice;
            std::string mLastTopic; // last topic ID, lowercase
            bool mIsInChoice;

            float mTemporaryDispositionChange;
            float mPermanentDispositionChange;

            void parseText (const std::string& text);

            void updateTopics();
            void updateGlobals();

            bool compile (const std::string& cmd,std::vector<Interpreter::Type_Code>& code);
            void executeScript (const std::string& script);

            void executeTopic (const std::string& topic);

        public:

            DialogueManager (const Compiler::Extensions& extensions, bool scriptVerbose, Translation::Storage& translationDataStorage);

            virtual void clear();

            virtual bool isInChoice() const;

            virtual void startDialogue (const MWWorld::Ptr& actor);

            virtual void addTopic (const std::string& topic);

            virtual void askQuestion (const std::string& question,int choice);

            virtual void goodbye();

            virtual bool checkServiceRefused ();

            virtual void say(const MWWorld::Ptr &actor, const std::string &topic) const;

            //calbacks for the GUI
            virtual void keywordSelected (const std::string& keyword);
            virtual void goodbyeSelected();
            virtual void questionAnswered (int answer);

            virtual void persuade (int type);
            virtual int getTemporaryDispositionChange () const;

            /// @note This change is temporary and gets discarded when dialogue ends.
            virtual void applyDispositionChange (int delta);

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

#ifndef GAME_MWDIALOG_DIALOGUEMANAGERIMP_H
#define GAME_MWDIALOG_DIALOGUEMANAGERIMP_H

#include "../mwbase/dialoguemanager.hpp"

#include <map>
#include <list>

#include <components/compiler/streamerrorhandler.hpp>

#include "../mwworld/ptr.hpp"

#include "../mwscript/compilercontext.hpp"

namespace MWDialogue
{
    class DialogueManager : public MWBase::DialogueManager
    {
            std::map<std::string, ESM::Dialogue> mDialogueMap;
            std::map<std::string, bool> mKnownTopics;// Those are the topics the player knows.
            std::list<std::string> mActorKnownTopics;

            MWScript::CompilerContext mCompilerContext;
            std::ostream mErrorStream;
            Compiler::StreamErrorHandler mErrorHandler;
            
            MWWorld::Ptr mActor;
            bool mTalkedTo;

            int mChoice;
            std::map<std::string, int> mChoiceMap;
            std::string mLastTopic;
            ESM::DialInfo mLastDialogue;
            bool mIsInChoice;

            float mTemporaryDispositionChange;
            float mPermanentDispositionChange;
            bool mScriptVerbose;

            void parseText (const std::string& text);

            void updateTopics();

            bool compile (const std::string& cmd,std::vector<Interpreter::Type_Code>& code);
            void executeScript (const std::string& script);

            void printError (const std::string& error);

        public:

            DialogueManager (const Compiler::Extensions& extensions, bool scriptVerbose);

            virtual void startDialogue (const MWWorld::Ptr& actor);

            virtual void addTopic (const std::string& topic);

            virtual void askQuestion (const std::string& question,int choice);

            virtual void goodbye();

            virtual MWWorld::Ptr getActor() const;
            ///< Return the actor the player is currently talking to.

            //calbacks for the GUI
            virtual void keywordSelected (const std::string& keyword);
            virtual void goodbyeSelected();
            virtual void questionAnswered (const std::string& answer);

            virtual void persuade (int type);
            virtual int getTemporaryDispositionChange () const;
            virtual void applyTemporaryDispositionChange (int delta);
    };
}

#endif

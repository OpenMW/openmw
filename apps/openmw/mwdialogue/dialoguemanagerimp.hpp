#ifndef GAME_MWDIALOG_DIALOGUEMANAGERIMP_H
#define GAME_MWDIALOG_DIALOGUEMANAGERIMP_H

#include "../mwbase/dialoguemanager.hpp"

#include <components/esm/loadinfo.hpp>

#include <components/compiler/streamerrorhandler.hpp>
#include "../mwscript/compilercontext.hpp"
#include "../mwscript/interpretercontext.hpp"
#include <components/compiler/output.hpp>

#include "../mwworld/ptr.hpp"

#include <map>

namespace MWDialogue
{
    class DialogueManager : public MWBase::DialogueManager
    {
            bool isMatching (const MWWorld::Ptr& actor, const ESM::DialInfo::SelectStruct& select) const;

            bool isMatching (const MWWorld::Ptr& actor, const ESM::DialInfo& info) const;

            bool functionFilter(const MWWorld::Ptr& actor, const ESM::DialInfo& info,bool choice);

            void parseText(std::string text);

            void updateTopics();

            std::map<std::string,ESM::Dialogue> mDialogueMap;
            std::map<std::string,bool> mKnownTopics;// Those are the topics the player knows.
            std::list<std::string> mActorKnownTopics;

            MWScript::CompilerContext mCompilerContext;
            std::ostream mErrorStream;
            Compiler::StreamErrorHandler mErrorHandler;


            bool compile (const std::string& cmd,std::vector<Interpreter::Type_Code>& code);
            void executeScript(std::string script);
            MWWorld::Ptr mActor;

            void printError(std::string error);

            int mChoice;
            std::map<std::string,int> mChoiceMap;
            std::string mLastTopic;
            ESM::DialInfo mLastDialogue;
            bool mIsInChoice;

            float mTemporaryDispositionChange;
            float mPermanentDispositionChange;

        public:

            DialogueManager (const Compiler::Extensions& extensions);

            virtual void startDialogue (const MWWorld::Ptr& actor);

            virtual void addTopic (const std::string& topic);

            virtual void askQuestion (const std::string& question,int choice);

            virtual void goodbye();

            ///get the faction of the actor you are talking with
            virtual std::string getFaction() const;

            //calbacks for the GUI
            virtual void keywordSelected (const std::string& keyword);
            virtual void goodbyeSelected();
            virtual void questionAnswered (const std::string& answer);

            virtual void persuade (int type);
            virtual int getTemporaryDispositionChange () const;
    };
}

#endif

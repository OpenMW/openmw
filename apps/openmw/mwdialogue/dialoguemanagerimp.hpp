#ifndef GAME_MWDIALOG_DIALOGUEMANAGERIMP_H
#define GAME_MWDIALOG_DIALOGUEMANAGERIMP_H

#include "../mwbase/dialoguemanager.hpp"

#include <map>
#include <list>

#include <components/compiler/streamerrorhandler.hpp>
#include <components/translation/translation.hpp>

#include "../mwworld/ptr.hpp"

#include "../mwscript/compilercontext.hpp"

namespace MWDialogue
{
    class DialogueManager : public MWBase::DialogueManager
    {
            std::map<std::string, ESM::Dialogue> mDialogueMap;
            std::map<std::string, bool> mKnownTopics;// Those are the topics the player knows.
            std::list<std::string> mActorKnownTopics;

            Translation::Storage& mTranslationDataStorage;
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

            void executeTopic (const std::string& topic, bool randomResponse=false);

        public:

            DialogueManager (const Compiler::Extensions& extensions, bool scriptVerbose, Translation::Storage& translationDataStorage);

            virtual void startDialogue (const MWWorld::Ptr& actor);

            virtual void addTopic (const std::string& topic);

            virtual void askQuestion (const std::string& question,int choice);

            virtual void goodbye();

            virtual MWWorld::Ptr getActor() const;
            ///< Return the actor the player is currently talking to.

            virtual bool checkServiceRefused ();

            //calbacks for the GUI
            virtual void keywordSelected (const std::string& keyword);
            virtual void goodbyeSelected();
            virtual void questionAnswered (const std::string& answer);

            virtual void persuade (int type);
            virtual int getTemporaryDispositionChange () const;
            virtual void applyTemporaryDispositionChange (int delta);
    };


    struct HyperTextToken
    {
        HyperTextToken(const std::string& text, bool link) : mText(text), mLink(link) {}

        std::string mText;
        bool mLink;
    };

    // In translations (at least Russian) the links are marked with @#, so
    // it should be a function to parse it
    std::vector<HyperTextToken> ParseHyperText(const std::string& text);

    size_t RemovePseudoAsterisks(std::string& phrase);
}

#endif

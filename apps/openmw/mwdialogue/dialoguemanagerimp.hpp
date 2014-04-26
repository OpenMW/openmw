#ifndef GAME_MWDIALOG_DIALOGUEMANAGERIMP_H
#define GAME_MWDIALOG_DIALOGUEMANAGERIMP_H

#include "../mwbase/dialoguemanager.hpp"

#include <map>
#include <list>

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
            std::map<std::string, bool> mKnownTopics;// Those are the topics the player knows.
            std::list<std::string> mActorKnownTopics;

            Translation::Storage& mTranslationDataStorage;
            MWScript::CompilerContext mCompilerContext;
            std::ostream mErrorStream;
            Compiler::StreamErrorHandler mErrorHandler;

            MWWorld::Ptr mActor;
            bool mTalkedTo;

            int mChoice;
            std::string mLastTopic;
            bool mIsInChoice;

            float mTemporaryDispositionChange;
            float mPermanentDispositionChange;
            bool mScriptVerbose;

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
            virtual void applyDispositionChange (int delta);

            virtual int countSavedGameRecords() const;

            virtual void write (ESM::ESMWriter& writer) const;

            virtual void readRecord (ESM::ESMReader& reader, int32_t type);
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

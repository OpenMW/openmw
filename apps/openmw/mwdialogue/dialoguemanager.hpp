#ifndef GAME_MMDIALOG_DIALOGUEMANAGER_H
#define GAME_MWDIALOG_DIALOGUEMANAGER_H

#include <components/esm/loadinfo.hpp>

#include <components/compiler/errorhandler.hpp>
#include "../mwscript/compilercontext.hpp"
#include "../mwscript/interpretercontext.hpp"
#include <components/compiler/output.hpp>

#include "../mwworld/ptr.hpp"
#include <map>

namespace MWWorld
{
    class Environment;
}

namespace MWDialogue
{
    class DialogueManager: private Compiler::ErrorHandler
    {
            MWWorld::Environment& mEnvironment;

            bool isMatching (const MWWorld::Ptr& actor, const ESM::DialInfo::SelectStruct& select) const;

            bool isMatching (const MWWorld::Ptr& actor, const ESM::DialInfo& info) const;

            void parseText(std::string text);

            std::map<std::string,bool> knownTopics;// Those are the topics the player knows.
            std::map<std::string,ESM::DialInfo> actorKnownTopics;

            MWScript::CompilerContext mCompilerContext;

            /// Report error to the user.
            virtual void report (const std::string& message, const Compiler::TokenLoc& loc, Type type){};

            /// Report a file related error
            virtual void report (const std::string& message, Type type){};

            bool compile (const std::string& cmd, Compiler::Output& output);
            void executeScript(std::string script);
            MWWorld::Ptr mActor;

            void printError(std::string error);

        public:

            DialogueManager (MWWorld::Environment& environment);

            void startDialogue (const MWWorld::Ptr& actor);

            void addTopic(std::string topic);

            //calbacks for the GUI
            void keywordSelected(std::string keyword);
            void goodbyeSelected();
            void questionAnswered(std::string answere);

    };
}

#endif

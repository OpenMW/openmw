#ifndef GAME_MMDIALOG_DIALOGUEMANAGER_H
#define GAME_MWDIALOG_DIALOGUEMANAGER_H

#include <components/esm/loadinfo.hpp>

#include "../mwworld/ptr.hpp"
#include <map>

namespace MWWorld
{
    class Environment;
}

namespace MWDialogue
{
    class DialogueManager
    {
            MWWorld::Environment& mEnvironment;

            bool isMatching (const MWWorld::Ptr& actor, const ESM::DialInfo::SelectStruct& select) const;

            bool isMatching (const MWWorld::Ptr& actor, const ESM::DialInfo& info) const;

            std::map<std::string,bool> knownTopics;// Those are the topics the player knows.
            std::map<std::string,std::string> actorKnownTopics;

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

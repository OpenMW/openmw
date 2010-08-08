#ifndef GAME_MMDIALOG_DIALOGUEMANAGER_H
#define GAME_MWDIALOG_DIALOGUEMANAGER_H

#include <components/esm/loadinfo.hpp>

#include "../mwworld/ptr.hpp"

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

        public:

            DialogueManager (MWWorld::Environment& environment);

            void startDialogue (const MWWorld::Ptr& actor);

    };
}

#endif

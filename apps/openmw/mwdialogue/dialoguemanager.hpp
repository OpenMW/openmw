#ifndef GAME_MMDIALOG_DIALOGUEMANAGER_H
#define GAME_MWDIALOG_DIALOGUEMANAGER_H

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

        public:

            DialogueManager (MWWorld::Environment& environment);

            void startDialogue (const MWWorld::Ptr& actor);

    };
}

#endif

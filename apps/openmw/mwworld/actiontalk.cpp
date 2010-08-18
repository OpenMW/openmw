
#include "actiontalk.hpp"

#include "environment.hpp"

#include "../mwdialogue/dialoguemanager.hpp"

namespace MWWorld
{
    ActionTalk::ActionTalk (const Ptr& actor) : mActor (actor) {}

    void ActionTalk::execute (Environment& environment)
    {
        environment.mDialogueManager->startDialogue (mActor);
    }
}

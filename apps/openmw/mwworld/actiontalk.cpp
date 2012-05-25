
#include "actiontalk.hpp"

#include "../mwbase/environment.hpp"
#include "../mwgui/window_manager.hpp"
#include "../mwdialogue/dialoguemanager.hpp"

namespace MWWorld
{
    ActionTalk::ActionTalk (const Ptr& actor) : mActor (actor) {}

    void ActionTalk::execute()
    {
        if (!MWBase::Environment::get().getWindowManager()->isAllowed(MWGui::GW_Inventory))
            return;

        MWBase::Environment::get().getDialogueManager()->startDialogue (mActor);
    }
}

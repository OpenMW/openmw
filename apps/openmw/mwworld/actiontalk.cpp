
#include "actiontalk.hpp"

#include "environment.hpp"

#include "../mwgui/window_manager.hpp"

namespace MWWorld
{
    ActionTalk::ActionTalk (const Ptr& actor) : mActor (actor) {}

    void ActionTalk::execute (Environment& environment)
    {
        environment.mWindowManager->setMode (MWGui::GM_Dialogue);
    }
}

#include "actionopen.hpp"

#include "environment.hpp"
#include "class.hpp"
#include "world.hpp"
#include "containerstore.hpp"
#include "../mwclass/container.hpp"
#include "../mwgui/window_manager.hpp"
#include "../mwgui/container.hpp"

namespace MWWorld
{
    ActionOpen::ActionOpen (const MWWorld::Ptr& container) : mContainer (container) {
        mContainer = container;
    }

    void ActionOpen::execute (Environment& environment)
    {
        environment.mWindowManager->setGuiMode(MWGui::GuiMode::GM_Container);
        environment.mWindowManager->getContainerWindow()->open(mContainer);
    }
}

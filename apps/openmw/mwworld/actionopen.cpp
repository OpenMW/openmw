#include "actionopen.hpp"

#include "../mwbase/environment.hpp"
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

    void ActionOpen::execute ()
    {
        MWBase::Environment::get().getWindowManager()->pushGuiMode(MWGui::GM_Container);
        MWBase::Environment::get().getWindowManager()->getContainerWindow()->open(mContainer);
    }
}

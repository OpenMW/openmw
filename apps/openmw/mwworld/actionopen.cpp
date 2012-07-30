#include "actionopen.hpp"

#include "../mwbase/environment.hpp"

#include "../mwclass/container.hpp"
#include "../mwgui/window_manager.hpp"
#include "../mwgui/container.hpp"

#include "class.hpp"
#include "containerstore.hpp"

namespace MWWorld
{
    ActionOpen::ActionOpen (const MWWorld::Ptr& container) : mContainer (container) {
        mContainer = container;
    }

    void ActionOpen::executeImp (const MWWorld::Ptr& actor)
    {
        if (!MWBase::Environment::get().getWindowManager()->isAllowed(MWGui::GW_Inventory))
            return;

        MWBase::Environment::get().getWindowManager()->pushGuiMode(MWGui::GM_Container);
        MWBase::Environment::get().getWindowManager()->getContainerWindow()->open(mContainer);
    }
}

#include "actionopen.hpp"

#include "../mwbase/environment.hpp"
#include "../mwbase/windowmanager.hpp"

#include "../mwgui/container.hpp"

#include "class.hpp"
#include "containerstore.hpp"

namespace MWWorld
{
    ActionOpen::ActionOpen (const MWWorld::Ptr& container, bool loot)
        : Action (false, container)
        , mLoot(loot)
    {
    }

    void ActionOpen::executeImp (const MWWorld::Ptr& actor)
    {
        if (!MWBase::Environment::get().getWindowManager()->isAllowed(MWGui::GW_Inventory))
            return;

        MWBase::Environment::get().getWindowManager()->pushGuiMode(MWGui::GM_Container);
        MWBase::Environment::get().getWindowManager()->getContainerWindow()->open(getTarget(), mLoot);
    }
}

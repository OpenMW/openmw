#include "actionrepair.hpp"

#include "../mwbase/environment.hpp"
#include "../mwbase/windowmanager.hpp"

namespace MWWorld
{
    ActionRepair::ActionRepair(const Ptr &item)
        : Action(false, item)
    {
    }

    void ActionRepair::executeImp (const Ptr& actor)
    {
        MWBase::Environment::get().getWindowManager()->pushGuiMode(MWGui::GM_Repair);
        MWBase::Environment::get().getWindowManager()->startRepairItem(getTarget());
    }
}

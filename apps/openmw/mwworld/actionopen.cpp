#include "actionopen.hpp"

#include "../mwbase/environment.hpp"
#include "../mwbase/windowmanager.hpp"

#include "../mwmechanics/disease.hpp"

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

        MWMechanics::diseaseContact(actor, getTarget());

        MWBase::Environment::get().getWindowManager()->openContainer(getTarget(), mLoot);
    }
}

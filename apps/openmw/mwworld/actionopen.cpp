#include "actionopen.hpp"

#include "../mwbase/environment.hpp"
#include "../mwbase/windowmanager.hpp"

#include "../mwmechanics/disease.hpp"

#include "class.hpp"
#include "containerstore.hpp"

namespace MWWorld
{
    ActionOpen::ActionOpen (const MWWorld::Ptr& container)
        : Action (false, container)
    {
    }

    void ActionOpen::executeImp (const MWWorld::Ptr& actor)
    {
        if (!MWBase::Environment::get().getWindowManager()->isAllowed(MWGui::GW_Inventory))
            return;

        MWMechanics::diseaseContact(actor, getTarget());

        MWBase::Environment::get().getWindowManager()->pushGuiMode(MWGui::GM_Container, getTarget());
    }
}

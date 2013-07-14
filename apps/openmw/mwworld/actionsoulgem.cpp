#include "actionsoulgem.hpp"

#include "../mwbase/windowmanager.hpp"
#include "../mwbase/environment.hpp"

namespace MWWorld
{

ActionSoulgem::ActionSoulgem(const Ptr &object)
    : Action(false, object)
{

}

void ActionSoulgem::executeImp(const Ptr &actor)
{
    MWBase::Environment::get().getWindowManager()->showSoulgemDialog(getTarget());
}


}

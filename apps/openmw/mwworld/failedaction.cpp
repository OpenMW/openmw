#include "failedaction.hpp"
#include "../mwbase/world.hpp"

#include "../mwbase/environment.hpp"
#include "../mwbase/windowmanager.hpp"


namespace MWWorld
{
    FailedAction::FailedAction(const std::string &msg)
      : Action(false), mMessage(msg)
    {   }

    void FailedAction::executeImp(const Ptr &actor)
    {
        if(actor == MWBase::Environment::get().getWorld()->getPlayerPtr() && !mMessage.empty())
            MWBase::Environment::get().getWindowManager()->messageBox(mMessage);
    }
}

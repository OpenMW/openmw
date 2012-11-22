#include "failedaction.hpp"
#include "../mwbase/world.hpp"

#include "../mwbase/environment.hpp"
#include "../mwbase/windowmanager.hpp"


namespace MWWorld
{
    FailedAction::FailedAction (const std::string& msg) : Action (false), message(msg)
    {   }


    void FailedAction::executeImp (const Ptr& actor)
    {
        if ( actor.getRefData().getHandle()=="player" && !(message.empty()))
	{
	  MWBase::Environment::get().getWindowManager() ->messageBox(message, std::vector<std::string>());
	}
    }
}

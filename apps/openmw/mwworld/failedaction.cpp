#include "failedaction.hpp"
#include "../mwbase/world.hpp"


namespace MWWorld
{
    FailedAction::FailedAction (const std::string& msg) : Action (false)
    {
      message = msg;
    }
    
    FailedAction::FailedAction () : Action (false)
    {

    }
    
    void FailedAction::executeImp (const Ptr& actor)
    {
        if ( actor.getRefData().getHandle()=="player" and not(message.empty()))
	{
	  //return a message here
	}
    }
}

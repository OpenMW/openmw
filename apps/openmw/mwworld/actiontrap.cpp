#include "actiontrap.hpp"

/*
    Start of tes3mp addition

    Include additional headers for multiplayer purposes
*/
#include "../mwmp/Main.hpp"
#include "../mwmp/Networking.hpp"
#include "../mwmp/WorldEvent.hpp"
/*
    End of tes3mp addition
*/

#include "../mwmechanics/spellcasting.hpp"

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"

namespace MWWorld
{
    void ActionTrap::executeImp(const Ptr &actor)
    {
        osg::Vec3f actorPosition(actor.getRefData().getPosition().asVec3());
        osg::Vec3f trapPosition(mTrapSource.getRefData().getPosition().asVec3());
        float trapRange = MWBase::Environment::get().getWorld()->getMaxActivationDistance();

        // Note: can't just detonate the trap at the trapped object's location and use the blast
        // radius, because for most trap spells this is 1 foot, much less than the activation distance.
        // Using activation distance as the trap range.

        if (actor == MWBase::Environment::get().getWorld()->getPlayerPtr() && MWBase::Environment::get().getWorld()->getDistanceToFacedObject() > trapRange) // player activated object outside range of trap
        {
            MWMechanics::CastSpell cast(mTrapSource, mTrapSource);
            cast.mHitPosition = trapPosition;
            cast.cast(mSpellId);
        }
        else // player activated object within range of trap, or NPC activated trap
        {
            MWMechanics::CastSpell cast(mTrapSource, actor);
            cast.mHitPosition = actorPosition;
            cast.cast(mSpellId);
        }   
        mTrapSource.getCellRef().setTrap("");

        /*
            Start of tes3mp addition

            Send an ID_OBJECT_TRAP packet every time an item is taken from the world
            by the player outside of the inventory screen
        */
        mwmp::WorldEvent *worldEvent = mwmp::Main::get().getNetworking()->getWorldEvent();
        worldEvent->reset();
        worldEvent->addObjectTrap(mTrapSource);
        worldEvent->sendObjectTrap();
        /*
            End of tes3mp addition
        */
    }
}

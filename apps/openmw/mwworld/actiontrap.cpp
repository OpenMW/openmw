#include "actiontrap.hpp"

#include "../mwmechanics/spellcasting.hpp"

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"

namespace MWWorld
{
    // actor activated object without telekinesis, so trap will hit
    void ActionTrap::executeImp(const Ptr &actor) 
    {
        osg::Vec3f actorPosition(actor.getRefData().getPosition().asVec3());
    
        MWMechanics::CastSpell cast(mTrapSource, actor);
        cast.mHitPosition = actorPosition;
        cast.cast(mSpellId);

        mTrapSource.getCellRef().setTrap("");
    }

    // actor activated object with telekinesis, so trap may or may not hit
    void ActionTrap::executeImp(const Ptr &actor, float distance)
    {
        osg::Vec3f trapPosition(mTrapSource.getRefData().getPosition().asVec3());
        float activationDistance = MWBase::Environment::get().getWorld()->getMaxActivationDistance();

        // Note: can't just detonate the trap at the trapped object's location and use the blast
        // radius, because for most trap spells this is 1 foot, much less than the activation distance.
        // Using activation distance as the trap range.

        if (distance < activationDistance) // actor activated object within range of trap
            executeImp(actor);
        else // actor activated object outside range of trap
        {
            MWMechanics::CastSpell cast(mTrapSource, mTrapSource);
            cast.mHitPosition = trapPosition;
            cast.cast(mSpellId);
            mTrapSource.getCellRef().setTrap("");
        }   
    }
}

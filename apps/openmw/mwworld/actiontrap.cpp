#include "actiontrap.hpp"

#include "../mwmechanics/spellcasting.hpp"
#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"

namespace MWWorld
{

    void ActionTrap::executeImp(const Ptr &actor)
    {
        osg::Vec3f actorPosition(actor.getRefData().getPosition().asVec3());
        osg::Vec3f trapPosition(mTrapSource.getRefData().getPosition().asVec3());
        float activationDistance = MWBase::Environment::get().getWorld()->getMaxActivationDistance();

        // GUI calcs if object in activation distance include object and player geometry
        const float fudgeFactor = 1.25f;

        // Hack: if actor is beyond activation range, then assume actor is using telekinesis
        // to open door/container.
        // Note, can't just detonate the trap at the trapped object's location and use the blast
        // radius, because for most trap spells this is 1 foot, much less than the activation distance.
        if ((trapPosition - actorPosition).length() < (activationDistance * fudgeFactor))
        {
            // assume actor touched trap
            MWMechanics::CastSpell cast(mTrapSource, actor);
            cast.mHitPosition = actorPosition;
            cast.cast(mSpellId);
        }
        else
        {
            // assume telekinesis used
            MWMechanics::CastSpell cast(mTrapSource, mTrapSource);
            cast.mHitPosition = trapPosition;
            cast.cast(mSpellId);
        }
        mTrapSource.getCellRef().setTrap("");
    }

}

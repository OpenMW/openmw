#include "actiontrap.hpp"

#include "../mwmechanics/spellcasting.hpp"

namespace MWWorld
{

    void ActionTrap::executeImp(const Ptr &actor)
    {
        MWMechanics::CastSpell cast(mTrapSource, actor);
        cast.mHitPosition = Ogre::Vector3(actor.getRefData().getPosition().pos);
        cast.cast(mSpellId);

        mTrapSource.getCellRef().setTrap("");
    }

}

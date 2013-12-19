#include "actiontrap.hpp"

#include "../mwmechanics/spellcasting.hpp"

namespace MWWorld
{

    void ActionTrap::executeImp(const Ptr &actor)
    {
        MWMechanics::CastSpell cast(mTrapSource, actor);
        cast.cast(mSpellId);

        mTrapSource.getCellRef().mTrap = "";
    }

}

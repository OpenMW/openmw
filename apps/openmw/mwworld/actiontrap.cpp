#include "actiontrap.hpp"

#include "../mwworld/class.hpp"

#include "../mwmechanics/activespells.hpp"
#include "../mwmechanics/creaturestats.hpp"

#include "../mwbase/world.hpp"
#include "../mwbase/environment.hpp"

namespace MWWorld
{

    void ActionTrap::executeImp(const Ptr &actor)
    {
        // TODO: Apply RT_Self effects on the door / container that triggered the trap. Not terribly useful, but you could
        // make it lock itself when activated for example.

        actor.getClass().getCreatureStats(actor).getActiveSpells().addSpell(mSpellId, actor, actor, ESM::RT_Touch);

        const ESM::Spell* spell = MWBase::Environment::get().getWorld()->getStore().get<ESM::Spell>().find(mSpellId);

        MWBase::Environment::get().getWorld()->launchProjectile(mSpellId, spell->mEffects, mTrapSource, spell->mName);

        mTrapSource.getCellRef().mTrap = "";
    }

}


#include "creature.hpp"

#include <components/esm/loadcrea.hpp>

#include "../mwmechanics/creaturestats.hpp"

#include "../mwworld/ptr.hpp"

namespace MWClass
{
    MWMechanics::CreatureStats& Creature::getCreatureStats (const MWWorld::Ptr& ptr) const
    {
        if (!ptr.getRefData().getCreatureStats().get())
        {
            boost::shared_ptr<MWMechanics::CreatureStats> stats (
                new MWMechanics::CreatureStats);

            ESMS::LiveCellRef<ESM::Creature, MWWorld::RefData> *ref = ptr.get<ESM::Creature>();

            stats->mAttributes[0].set (ref->base->data.strength);
            stats->mAttributes[1].set (ref->base->data.intelligence);
            stats->mAttributes[2].set (ref->base->data.willpower);
            stats->mAttributes[3].set (ref->base->data.agility);
            stats->mAttributes[4].set (ref->base->data.speed);
            stats->mAttributes[5].set (ref->base->data.endurance);
            stats->mAttributes[6].set (ref->base->data.personality);
            stats->mAttributes[7].set (ref->base->data.luck);
            stats->mDynamic[0].set (ref->base->data.health);
            stats->mDynamic[1].set (ref->base->data.mana);
            stats->mDynamic[2].set (ref->base->data.fatigue);

            ptr.getRefData().getCreatureStats() = stats;
        }

        return *ptr.getRefData().getCreatureStats();
    }

    void Creature::registerSelf()
    {
        boost::shared_ptr<Class> instance (new Creature);

        registerClass (typeid (ESM::Creature).name(), instance);
    }
}

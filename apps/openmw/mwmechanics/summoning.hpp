#ifndef OPENMW_MECHANICS_SUMMONING_H
#define OPENMW_MECHANICS_SUMMONING_H

#include <set>

#include "../mwworld/ptr.hpp"

#include <components/esm3/magiceffects.hpp>

#include "magiceffects.hpp"

namespace MWMechanics
{
    bool isSummoningEffect(int effectId);

    std::string getSummonedCreature(int effectId);

    void purgeSummonEffect(const MWWorld::Ptr& summoner, const std::pair<int, int>& summon);

    int summonCreature(int effectId, const MWWorld::Ptr& summoner);

    void updateSummons(const MWWorld::Ptr& summoner, bool cleanup);
}

#endif

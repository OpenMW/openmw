#ifndef OPENMW_MECHANICS_SUMMONING_H
#define OPENMW_MECHANICS_SUMMONING_H

#include <string_view>
#include <utility>

#include <components/esm3/magiceffects.hpp>

namespace MWWorld
{
    class Ptr;
}

namespace MWMechanics
{
    bool isSummoningEffect(int effectId);

    std::string_view getSummonedCreature(int effectId);

    void purgeSummonEffect(const MWWorld::Ptr& summoner, const std::pair<int, int>& summon);

    int summonCreature(int effectId, const MWWorld::Ptr& summoner);

    void updateSummons(const MWWorld::Ptr& summoner, bool cleanup);
}

#endif

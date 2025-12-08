#ifndef OPENMW_MECHANICS_SUMMONING_H
#define OPENMW_MECHANICS_SUMMONING_H

#include <string_view>
#include <utility>

#include <components/esm3/refnum.hpp>

namespace ESM
{
    class RefId;
    class StringRefId;
}
namespace MWWorld
{
    class Ptr;
}

namespace MWMechanics
{
    bool isSummoningEffect(const ESM::RefId& effectId);

    ESM::RefId getSummonedCreature(const ESM::RefId& effectId);

    void purgeSummonEffect(const MWWorld::Ptr& summoner, const std::pair<ESM::RefId, ESM::RefNum>& summon);

    ESM::RefNum summonCreature(const ESM::RefId& effectId, const MWWorld::Ptr& summoner);

    void updateSummons(const MWWorld::Ptr& summoner, bool cleanup);
}

#endif

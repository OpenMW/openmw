#ifndef OPENMW_MECHANICS_SUMMONING_H
#define OPENMW_MECHANICS_SUMMONING_H

#include <string_view>
#include <utility>

#include <components/esm3/refnum.hpp>

namespace ESM
{
    class RefId;
    class StringRefId;

    using MagicEffectId = StringRefId;
}
namespace MWWorld
{
    class Ptr;
}

namespace MWMechanics
{
    bool isSummoningEffect(const ESM::MagicEffectId& effectId);

    ESM::RefId getSummonedCreature(const ESM::MagicEffectId& effectId);

    void purgeSummonEffect(const MWWorld::Ptr& summoner, const std::pair<ESM::MagicEffectId, ESM::RefNum>& summon);

    ESM::RefNum summonCreature(const ESM::MagicEffectId& effectId, const MWWorld::Ptr& summoner);

    void updateSummons(const MWWorld::Ptr& summoner, bool cleanup);
}

#endif

#ifndef OPENMW_MECHANICS_SUMMONING_H
#define OPENMW_MECHANICS_SUMMONING_H

#include <set>

#include "../mwworld/ptr.hpp"

#include <components/esm/magiceffects.hpp>

#include "magiceffects.hpp"

namespace MWMechanics
{
    class CreatureStats;

    bool isSummoningEffect(int effectId);

    std::string getSummonedCreature(int effectId);

    void purgeSummonEffect(const MWWorld::Ptr& summoner, const std::pair<const ESM::SummonKey, int>& summon);

    struct UpdateSummonedCreatures : public EffectSourceVisitor
    {
        UpdateSummonedCreatures(const MWWorld::Ptr& actor);
        virtual ~UpdateSummonedCreatures() = default;

        void visit (MWMechanics::EffectKey key, int effectIndex,
                            const std::string& sourceName, const std::string& sourceId, int casterActorId,
                            float magnitude, float remainingTime = -1, float totalTime = -1) override;

        /// To call after all effect sources have been visited
        void process(bool cleanup);

    private:
        MWWorld::Ptr mActor;

        std::set<ESM::SummonKey> mActiveEffects;
    };

}

#endif

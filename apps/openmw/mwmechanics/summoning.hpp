#ifndef OPENMW_MECHANICS_SUMMONING_H
#define OPENMW_MECHANICS_SUMMONING_H

#include <set>

#include "../mwworld/ptr.hpp"

#include "magiceffects.hpp"

namespace MWMechanics
{
    class CreatureStats;

    bool isSummoningEffect(int effectId);

    std::string getSummonedCreature(int effectId);

    struct UpdateSummonedCreatures : public EffectSourceVisitor
    {
        UpdateSummonedCreatures(const MWWorld::Ptr& actor);
        virtual ~UpdateSummonedCreatures() = default;

        virtual void visit (MWMechanics::EffectKey key,
                                 const std::string& sourceName, const std::string& sourceId, int casterActorId,
                            float magnitude, float remainingTime = -1, float totalTime = -1);

        /// To call after all effect sources have been visited
        void process(bool cleanup);

    private:
        MWWorld::Ptr mActor;

        std::set<std::pair<int, std::string> > mActiveEffects;
    };

}

#endif

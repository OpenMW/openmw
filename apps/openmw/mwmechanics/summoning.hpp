#ifndef OPENMW_MECHANICS_SUMMONING_H
#define OPENMW_MECHANICS_SUMMONING_H

#include <set>

#include "magiceffects.hpp"
#include "../mwworld/ptr.hpp"

namespace MWMechanics
{

    class CreatureStats;

    struct UpdateSummonedCreatures : public EffectSourceVisitor
    {
        UpdateSummonedCreatures(const MWWorld::Ptr& actor);
        virtual ~UpdateSummonedCreatures();

        virtual void visit (MWMechanics::EffectKey key,
                                 const std::string& sourceName, const std::string& sourceId, int casterActorId,
                            float magnitude, float remainingTime = -1, float totalTime = -1);

        /// To call after all effect sources have been visited
        void finish();

    private:
        MWWorld::Ptr mActor;

        std::set<std::pair<int, std::string> > mActiveEffects;
    };

    void cleanupSummonedCreature (MWMechanics::CreatureStats& casterStats, int creatureActorId);

}

#endif

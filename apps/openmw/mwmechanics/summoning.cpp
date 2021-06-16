#include "summoning.hpp"

#include <components/debug/debuglog.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwbase/mechanicsmanager.hpp"

#include "../mwworld/esmstore.hpp"
#include "../mwworld/class.hpp"
#include "../mwworld/manualref.hpp"
#include "../mwworld/inventorystore.hpp"

#include "../mwrender/animation.hpp"

#include "creaturestats.hpp"
#include "aifollow.hpp"

namespace MWMechanics
{

    bool isSummoningEffect(int effectId)
    {
        return ((effectId >= ESM::MagicEffect::SummonScamp && effectId <= ESM::MagicEffect::SummonStormAtronach)
             || (effectId == ESM::MagicEffect::SummonCenturionSphere)
             || (effectId >= ESM::MagicEffect::SummonFabricant && effectId <= ESM::MagicEffect::SummonCreature05));
    }

    std::string getSummonedCreature(int effectId)
    {
        static const std::map<int, std::string> summonMap
        {
            {ESM::MagicEffect::SummonAncestralGhost, "sMagicAncestralGhostID"},
            {ESM::MagicEffect::SummonBonelord, "sMagicBonelordID"},
            {ESM::MagicEffect::SummonBonewalker, "sMagicLeastBonewalkerID"},
            {ESM::MagicEffect::SummonCenturionSphere, "sMagicCenturionSphereID"},
            {ESM::MagicEffect::SummonClannfear, "sMagicClannfearID"},
            {ESM::MagicEffect::SummonDaedroth, "sMagicDaedrothID"},
            {ESM::MagicEffect::SummonDremora, "sMagicDremoraID"},
            {ESM::MagicEffect::SummonFabricant, "sMagicFabricantID"},
            {ESM::MagicEffect::SummonFlameAtronach, "sMagicFlameAtronachID"},
            {ESM::MagicEffect::SummonFrostAtronach, "sMagicFrostAtronachID"},
            {ESM::MagicEffect::SummonGoldenSaint, "sMagicGoldenSaintID"},
            {ESM::MagicEffect::SummonGreaterBonewalker, "sMagicGreaterBonewalkerID"},
            {ESM::MagicEffect::SummonHunger, "sMagicHungerID"},
            {ESM::MagicEffect::SummonScamp, "sMagicScampID"},
            {ESM::MagicEffect::SummonSkeletalMinion, "sMagicSkeletalMinionID"},
            {ESM::MagicEffect::SummonStormAtronach, "sMagicStormAtronachID"},
            {ESM::MagicEffect::SummonWingedTwilight, "sMagicWingedTwilightID"},
            {ESM::MagicEffect::SummonWolf, "sMagicCreature01ID"},
            {ESM::MagicEffect::SummonBear, "sMagicCreature02ID"},
            {ESM::MagicEffect::SummonBonewolf, "sMagicCreature03ID"},
            {ESM::MagicEffect::SummonCreature04, "sMagicCreature04ID"},
            {ESM::MagicEffect::SummonCreature05, "sMagicCreature05ID"}
        };

        auto it = summonMap.find(effectId);
        if (it != summonMap.end())
            return MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>().find(it->second)->mValue.getString();
        return std::string();
    }

    UpdateSummonedCreatures::UpdateSummonedCreatures(const MWWorld::Ptr &actor)
        : mActor(actor)
    {
    }

    void UpdateSummonedCreatures::visit(EffectKey key, int effectIndex, const std::string &sourceName, const std::string &sourceId, int casterActorId, float magnitude, float remainingTime, float totalTime)
    {
        if (isSummoningEffect(key.mId) && magnitude > 0)
        {
            mActiveEffects.insert(ESM::SummonKey(key.mId, sourceId, effectIndex));
        }
    }

    void UpdateSummonedCreatures::process(bool cleanup)
    {
        MWMechanics::CreatureStats& creatureStats = mActor.getClass().getCreatureStats(mActor);
        std::map<ESM::SummonKey, int>& creatureMap = creatureStats.getSummonedCreatureMap();

        for (std::set<ESM::SummonKey>::iterator it = mActiveEffects.begin(); it != mActiveEffects.end(); ++it)
        {
            bool found = creatureMap.find(*it) != creatureMap.end();
            if (!found)
            {
                std::string creatureID = getSummonedCreature(it->mEffectId);
                if (!creatureID.empty())
                {
                    int creatureActorId = -1;
                    try
                    {
                        MWWorld::ManualRef ref(MWBase::Environment::get().getWorld()->getStore(), creatureID, 1);

                        MWMechanics::CreatureStats& summonedCreatureStats = ref.getPtr().getClass().getCreatureStats(ref.getPtr());

                        // Make the summoned creature follow its master and help in fights
                        AiFollow package(mActor);
                        summonedCreatureStats.getAiSequence().stack(package, ref.getPtr());
                        creatureActorId = summonedCreatureStats.getActorId();

                        MWWorld::Ptr placed = MWBase::Environment::get().getWorld()->safePlaceObject(ref.getPtr(), mActor, mActor.getCell(), 0, 120.f);

                        MWRender::Animation* anim = MWBase::Environment::get().getWorld()->getAnimation(placed);
                        if (anim)
                        {
                            const ESM::Static* fx = MWBase::Environment::get().getWorld()->getStore().get<ESM::Static>()
                                    .search("VFX_Summon_Start");
                            if (fx)
                                anim->addEffect("meshes\\" + fx->mModel, -1, false);
                        }
                    }
                    catch (std::exception& e)
                    {
                        Log(Debug::Error) << "Failed to spawn summoned creature: " << e.what();
                        // still insert into creatureMap so we don't try to spawn again every frame, that would spam the warning log
                    }

                    creatureMap.emplace(*it, creatureActorId);
                }
            }
        }

        // Update summon effects
        for (std::map<ESM::SummonKey, int>::iterator it = creatureMap.begin(); it != creatureMap.end(); )
        {
            bool found = mActiveEffects.find(it->first) != mActiveEffects.end();
            if (!found)
            {
                // Effect has ended
                MWBase::Environment::get().getMechanicsManager()->cleanupSummonedCreature(mActor, it->second);
                creatureMap.erase(it++);
                continue;
            }
            ++it;
        }

        std::vector<int> graveyard = creatureStats.getSummonedCreatureGraveyard();
        creatureStats.getSummonedCreatureGraveyard().clear();

        for (const int creature : graveyard)
            MWBase::Environment::get().getMechanicsManager()->cleanupSummonedCreature(mActor, creature);

        if (!cleanup)
            return;

        for (std::map<ESM::SummonKey, int>::iterator it = creatureMap.begin(); it != creatureMap.end(); )
        {
            if(it->second == -1)
            {
                // Keep the spell effect active if we failed to spawn anything
                it++;
                continue;
            }
            MWWorld::Ptr ptr = MWBase::Environment::get().getWorld()->searchPtrViaActorId(it->second);
            if (!ptr.isEmpty() && ptr.getClass().getCreatureStats(ptr).isDead() && ptr.getClass().getCreatureStats(ptr).isDeathAnimationFinished())
            {
                // Purge the magic effect so a new creature can be summoned if desired
                purgeSummonEffect(mActor, *it);
                creatureMap.erase(it++);
            }
            else
                ++it;
        }
    }

    void purgeSummonEffect(const MWWorld::Ptr& summoner, const std::pair<const ESM::SummonKey, int>& summon)
    {
        auto& creatureStats = summoner.getClass().getCreatureStats(summoner);
        creatureStats.getActiveSpells().purgeEffect(summon.first.mEffectId, summon.first.mSourceId, summon.first.mEffectIndex);
        creatureStats.getSpells().purgeEffect(summon.first.mEffectId, summon.first.mSourceId);
        if (summoner.getClass().hasInventoryStore(summoner))
            summoner.getClass().getInventoryStore(summoner).purgeEffect(summon.first.mEffectId, summon.first.mSourceId, false, summon.first.mEffectIndex);

        MWBase::Environment::get().getMechanicsManager()->cleanupSummonedCreature(summoner, summon.second);
    }
}

#include "summoning.hpp"

#include <iostream>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwbase/mechanicsmanager.hpp"

#include "../mwworld/esmstore.hpp"
#include "../mwworld/class.hpp"
#include "../mwworld/manualref.hpp"
#include "../mwworld/inventorystore.hpp"

#include "../mwrender/animation.hpp"

#include "spellcasting.hpp"
#include "creaturestats.hpp"
#include "aifollow.hpp"

namespace MWMechanics
{

    UpdateSummonedCreatures::UpdateSummonedCreatures(const MWWorld::Ptr &actor)
        : mActor(actor)
    {

    }

    UpdateSummonedCreatures::~UpdateSummonedCreatures()
    {
    }

    void UpdateSummonedCreatures::visit(EffectKey key, const std::string &sourceName, const std::string &sourceId, int casterActorId, float magnitude, float remainingTime, float totalTime)
    {
        if (isSummoningEffect(key.mId) && magnitude > 0)
        {
            mActiveEffects.insert(std::make_pair(key.mId, sourceId));
        }
    }

    void UpdateSummonedCreatures::process()
    {
        static std::map<int, std::string> summonMap;
        if (summonMap.empty())
        {
            summonMap[ESM::MagicEffect::SummonAncestralGhost] = "sMagicAncestralGhostID";
            summonMap[ESM::MagicEffect::SummonBonelord] = "sMagicBonelordID";
            summonMap[ESM::MagicEffect::SummonBonewalker] = "sMagicLeastBonewalkerID";
            summonMap[ESM::MagicEffect::SummonCenturionSphere] = "sMagicCenturionSphereID";
            summonMap[ESM::MagicEffect::SummonClannfear] = "sMagicClannfearID";
            summonMap[ESM::MagicEffect::SummonDaedroth] = "sMagicDaedrothID";
            summonMap[ESM::MagicEffect::SummonDremora] = "sMagicDremoraID";
            summonMap[ESM::MagicEffect::SummonFabricant] = "sMagicFabricantID";
            summonMap[ESM::MagicEffect::SummonFlameAtronach] = "sMagicFlameAtronachID";
            summonMap[ESM::MagicEffect::SummonFrostAtronach] = "sMagicFrostAtronachID";
            summonMap[ESM::MagicEffect::SummonGoldenSaint] = "sMagicGoldenSaintID";
            summonMap[ESM::MagicEffect::SummonGreaterBonewalker] = "sMagicGreaterBonewalkerID";
            summonMap[ESM::MagicEffect::SummonHunger] = "sMagicHungerID";
            summonMap[ESM::MagicEffect::SummonScamp] = "sMagicScampID";
            summonMap[ESM::MagicEffect::SummonSkeletalMinion] = "sMagicSkeletalMinionID";
            summonMap[ESM::MagicEffect::SummonStormAtronach] = "sMagicStormAtronachID";
            summonMap[ESM::MagicEffect::SummonWingedTwilight] = "sMagicWingedTwilightID";
            summonMap[ESM::MagicEffect::SummonWolf] = "sMagicCreature01ID";
            summonMap[ESM::MagicEffect::SummonBear] = "sMagicCreature02ID";
            summonMap[ESM::MagicEffect::SummonBonewolf] = "sMagicCreature03ID";
            summonMap[ESM::MagicEffect::SummonCreature04] = "sMagicCreature04ID";
            summonMap[ESM::MagicEffect::SummonCreature05] = "sMagicCreature05ID";
        }

        MWMechanics::CreatureStats& creatureStats = mActor.getClass().getCreatureStats(mActor);

        // Update summon effects
        std::map<CreatureStats::SummonKey, int>& creatureMap = creatureStats.getSummonedCreatureMap();
        for (std::map<CreatureStats::SummonKey, int>::iterator it = creatureMap.begin(); it != creatureMap.end(); )
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

        for (std::set<std::pair<int, std::string> >::iterator it = mActiveEffects.begin(); it != mActiveEffects.end(); ++it)
        {
            bool found = creatureMap.find(std::make_pair(it->first, it->second)) != creatureMap.end();
            if (!found)
            {
                const std::string& creatureGmst = summonMap[it->first];
                std::string creatureID =
                        MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>().find(creatureGmst)->getString();

                if (!creatureID.empty())
                {
                    int creatureActorId = -1;
                    try
                    {
                        MWWorld::ManualRef ref(MWBase::Environment::get().getWorld()->getStore(), creatureID, 1);

                        MWMechanics::CreatureStats& summonedCreatureStats = ref.getPtr().getClass().getCreatureStats(ref.getPtr());

                        // Make the summoned creature follow its master and help in fights
                        AiFollow package(mActor.getCellRef().getRefId());
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
                        std::cerr << "Failed to spawn summoned creature: " << e.what() << std::endl;
                        // still insert into creatureMap so we don't try to spawn again every frame, that would spam the warning log
                    }

                    creatureMap.insert(std::make_pair(*it, creatureActorId));
                }
            }
        }

        for (std::map<CreatureStats::SummonKey, int>::iterator it = creatureMap.begin(); it != creatureMap.end(); )
        {
            MWWorld::Ptr ptr = MWBase::Environment::get().getWorld()->searchPtrViaActorId(it->second);
            if (!ptr.isEmpty() && ptr.getClass().getCreatureStats(ptr).isDead() && ptr.getClass().getCreatureStats(ptr).isDeathAnimationFinished())
            {
                // Purge the magic effect so a new creature can be summoned if desired
                creatureStats.getActiveSpells().purgeEffect(it->first.first, it->first.second);
                if (mActor.getClass().hasInventoryStore(ptr))
                    mActor.getClass().getInventoryStore(mActor).purgeEffect(it->first.first, it->first.second);

                MWBase::Environment::get().getMechanicsManager()->cleanupSummonedCreature(mActor, it->second);
                creatureMap.erase(it++);
            }
            else
                ++it;
        }

        std::vector<int>& graveyard = creatureStats.getSummonedCreatureGraveyard();
        for (std::vector<int>::iterator it = graveyard.begin(); it != graveyard.end(); )
        {
            MWWorld::Ptr ptr = MWBase::Environment::get().getWorld()->searchPtrViaActorId(*it);
            if (!ptr.isEmpty())
            {
                it = graveyard.erase(it);

                const ESM::Static* fx = MWBase::Environment::get().getWorld()->getStore().get<ESM::Static>()
                        .search("VFX_Summon_End");
                if (fx)
                    MWBase::Environment::get().getWorld()->spawnEffect("meshes\\" + fx->mModel,
                        "", ptr.getRefData().getPosition().asVec3());

                MWBase::Environment::get().getWorld()->deleteObject(ptr);
            }
            else
                ++it;
        }
    }

}

#include "summoning.hpp"

#include <components/debug/debuglog.hpp>
#include <components/esm/refid.hpp>
#include <components/esm3/loadmgef.hpp>
#include <components/esm3/loadstat.hpp>
#include <components/misc/resourcehelpers.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/mechanicsmanager.hpp"
#include "../mwbase/world.hpp"

#include "../mwworld/class.hpp"
#include "../mwworld/esmstore.hpp"
#include "../mwworld/manualref.hpp"

#include "../mwrender/animation.hpp"

#include "aifollow.hpp"
#include "creaturestats.hpp"

namespace MWMechanics
{

    bool isSummoningEffect(int effectId)
    {
        return ((effectId >= ESM::MagicEffect::SummonScamp && effectId <= ESM::MagicEffect::SummonStormAtronach)
            || (effectId == ESM::MagicEffect::SummonCenturionSphere)
            || (effectId >= ESM::MagicEffect::SummonFabricant && effectId <= ESM::MagicEffect::SummonCreature05));
    }

    static const std::map<int, ESM::RefId>& getSummonMap()
    {
        static std::map<int, ESM::RefId> summonMap;

        if (summonMap.size() > 0)
            return summonMap;

        const std::map<int, std::string_view> summonMapToGameSetting{
            { ESM::MagicEffect::SummonAncestralGhost, "sMagicAncestralGhostID" },
            { ESM::MagicEffect::SummonBonelord, "sMagicBonelordID" },
            { ESM::MagicEffect::SummonBonewalker, "sMagicLeastBonewalkerID" },
            { ESM::MagicEffect::SummonCenturionSphere, "sMagicCenturionSphereID" },
            { ESM::MagicEffect::SummonClannfear, "sMagicClannfearID" },
            { ESM::MagicEffect::SummonDaedroth, "sMagicDaedrothID" },
            { ESM::MagicEffect::SummonDremora, "sMagicDremoraID" },
            { ESM::MagicEffect::SummonFabricant, "sMagicFabricantID" },
            { ESM::MagicEffect::SummonFlameAtronach, "sMagicFlameAtronachID" },
            { ESM::MagicEffect::SummonFrostAtronach, "sMagicFrostAtronachID" },
            { ESM::MagicEffect::SummonGoldenSaint, "sMagicGoldenSaintID" },
            { ESM::MagicEffect::SummonGreaterBonewalker, "sMagicGreaterBonewalkerID" },
            { ESM::MagicEffect::SummonHunger, "sMagicHungerID" },
            { ESM::MagicEffect::SummonScamp, "sMagicScampID" },
            { ESM::MagicEffect::SummonSkeletalMinion, "sMagicSkeletalMinionID" },
            { ESM::MagicEffect::SummonStormAtronach, "sMagicStormAtronachID" },
            { ESM::MagicEffect::SummonWingedTwilight, "sMagicWingedTwilightID" },
            { ESM::MagicEffect::SummonWolf, "sMagicCreature01ID" },
            { ESM::MagicEffect::SummonBear, "sMagicCreature02ID" },
            { ESM::MagicEffect::SummonBonewolf, "sMagicCreature03ID" },
            { ESM::MagicEffect::SummonCreature04, "sMagicCreature04ID" },
            { ESM::MagicEffect::SummonCreature05, "sMagicCreature05ID" },
        };

        for (const auto& it : summonMapToGameSetting)
        {
            summonMap[it.first] = ESM::RefId::stringRefId(
                MWBase::Environment::get().getESMStore()->get<ESM::GameSetting>().find(it.second)->mValue.getString());
        }
        return summonMap;
    }

    ESM::RefId getSummonedCreature(int effectId)
    {
        const auto& summonMap = getSummonMap();
        auto it = summonMap.find(effectId);
        if (it != summonMap.end())
        {
            return it->second;
        }
        return ESM::RefId();
    }

    int summonCreature(int effectId, const MWWorld::Ptr& summoner)
    {
        const ESM::RefId& creatureID = getSummonedCreature(effectId);
        int creatureActorId = -1;
        if (!creatureID.empty())
        {
            try
            {
                auto world = MWBase::Environment::get().getWorld();
                MWWorld::ManualRef ref(world->getStore(), creatureID, 1);
                MWWorld::Ptr placed = world->safePlaceObject(ref.getPtr(), summoner, summoner.getCell(), 0, 120.f);

                MWMechanics::CreatureStats& summonedCreatureStats = placed.getClass().getCreatureStats(placed);

                // Make the summoned creature follow its master and help in fights
                AiFollow package(summoner);
                summonedCreatureStats.getAiSequence().stack(package, placed);
                creatureActorId = summonedCreatureStats.getActorId();

                MWRender::Animation* anim = world->getAnimation(placed);
                if (anim)
                {
                    const ESM::Static* fx
                        = world->getStore().get<ESM::Static>().search(ESM::RefId::stringRefId("VFX_Summon_Start"));
                    if (fx)
                        anim->addEffect(
                            Misc::ResourceHelpers::correctMeshPath(VFS::Path::Normalized(fx->mModel)).value(), "",
                            false);
                }
            }
            catch (std::exception& e)
            {
                Log(Debug::Error) << "Failed to spawn summoned creature: " << e.what();
                // still insert into creatureMap so we don't try to spawn again every frame, that would spam the warning
                // log
            }

            summoner.getClass().getCreatureStats(summoner).getSummonedCreatureMap().emplace(effectId, creatureActorId);
        }
        return creatureActorId;
    }

    void updateSummons(const MWWorld::Ptr& summoner, bool cleanup)
    {
        MWMechanics::CreatureStats& creatureStats = summoner.getClass().getCreatureStats(summoner);
        auto& creatureMap = creatureStats.getSummonedCreatureMap();

        std::vector<int> graveyard = creatureStats.getSummonedCreatureGraveyard();
        creatureStats.getSummonedCreatureGraveyard().clear();

        for (const int creature : graveyard)
            MWBase::Environment::get().getMechanicsManager()->cleanupSummonedCreature(summoner, creature);

        if (!cleanup)
            return;

        for (auto it = creatureMap.begin(); it != creatureMap.end();)
        {
            if (it->second == -1)
            {
                // Keep the spell effect active if we failed to spawn anything
                it++;
                continue;
            }
            MWWorld::Ptr ptr = MWBase::Environment::get().getWorld()->searchPtrViaActorId(it->second);
            if (!ptr.isEmpty() && ptr.getClass().getCreatureStats(ptr).isDead()
                && ptr.getClass().getCreatureStats(ptr).isDeathAnimationFinished())
            {
                // Purge the magic effect so a new creature can be summoned if desired
                auto summon = *it;
                creatureMap.erase(it++);
                purgeSummonEffect(summoner, summon);
            }
            else
                ++it;
        }
    }

    void purgeSummonEffect(const MWWorld::Ptr& summoner, const std::pair<int, int>& summon)
    {
        auto& creatureStats = summoner.getClass().getCreatureStats(summoner);
        creatureStats.getActiveSpells().purge(
            [summon](const auto& spell, const auto& effect) {
                return effect.mEffectId == summon.first && effect.getActorId() == summon.second;
            },
            summoner);

        MWBase::Environment::get().getMechanicsManager()->cleanupSummonedCreature(summoner, summon.second);
    }
}

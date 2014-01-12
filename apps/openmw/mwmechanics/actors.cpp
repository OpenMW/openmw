
#include "actors.hpp"

#include <typeinfo>

#include <OgreVector3.h>

#include <components/esm/loadnpc.hpp>

#include "../mwworld/esmstore.hpp"

#include "../mwworld/class.hpp"
#include "../mwworld/inventorystore.hpp"
#include "../mwworld/manualref.hpp"
#include "../mwworld/actionequip.hpp"

#include "../mwbase/world.hpp"
#include "../mwbase/environment.hpp"
#include "../mwbase/windowmanager.hpp"
#include "../mwbase/soundmanager.hpp"

#include "npcstats.hpp"
#include "creaturestats.hpp"
#include "movement.hpp"

#include "../mwbase/environment.hpp"
#include "../mwbase/mechanicsmanager.hpp"

#include "aicombat.hpp"

namespace
{

void adjustBoundItem (const std::string& item, bool bound, const MWWorld::Ptr& actor)
{
    if (bound)
    {
        MWWorld::Ptr newPtr = *actor.getClass().getContainerStore(actor).add(item, 1, actor);
        MWWorld::ActionEquip action(newPtr);
        action.execute(actor);
    }
    else
    {
        actor.getClass().getContainerStore(actor).remove(item, 1, actor);
    }
}

bool disintegrateSlot (MWWorld::Ptr ptr, int slot, float disintegrate)
{
    // TODO: remove this check once creatures support inventory store
    if (ptr.getTypeName() == typeid(ESM::NPC).name())
    {
        MWWorld::InventoryStore& inv = ptr.getClass().getInventoryStore(ptr);
        MWWorld::ContainerStoreIterator item =
                inv.getSlot(slot);
        if (item != inv.end())
        {
            if (!item->getClass().hasItemHealth(*item))
                return false;
            if (item->getCellRef().mCharge == -1)
                item->getCellRef().mCharge = item->getClass().getItemMaxHealth(*item);

            if (item->getCellRef().mCharge == 0)
                return false;

            item->getCellRef().mCharge -=
                    std::min(disintegrate,
                             static_cast<float>(item->getCellRef().mCharge));

            if (item->getCellRef().mCharge == 0)
            {
                // Will unequip the broken item and try to find a replacement
                if (ptr.getRefData().getHandle() != "player")
                    inv.autoEquip(ptr);
                else
                    inv.unequipItem(*item, ptr);
            }

            return true;
        }
    }
    return false;
}


}

namespace MWMechanics
{

    class SoulTrap : public MWMechanics::EffectSourceVisitor
    {
        MWWorld::Ptr mCreature;
        MWWorld::Ptr mActor;
    public:
        SoulTrap(MWWorld::Ptr trappedCreature)
            : mCreature(trappedCreature) {}

        virtual void visit (MWMechanics::EffectKey key,
                                 const std::string& sourceName, const std::string& casterHandle,
                            float magnitude, float remainingTime = -1)
        {
            if (key.mId != ESM::MagicEffect::Soultrap)
                return;
            if (magnitude <= 0)
                return;

            MWBase::World* world = MWBase::Environment::get().getWorld();

            MWWorld::Ptr caster = world->searchPtrViaHandle(casterHandle);
            if (caster.isEmpty() || !caster.getClass().isActor())
                return;

            static const float fSoulgemMult = world->getStore().get<ESM::GameSetting>().find("fSoulgemMult")->getFloat();

            float creatureSoulValue = mCreature.get<ESM::Creature>()->mBase->mData.mSoul;

            // Use the smallest soulgem that is large enough to hold the soul
            MWWorld::ContainerStore& container = caster.getClass().getContainerStore(caster);
            MWWorld::ContainerStoreIterator gem = container.end();
            float gemCapacity = std::numeric_limits<float>().max();
            std::string soulgemFilter = "misc_soulgem"; // no other way to check for soulgems? :/
            for (MWWorld::ContainerStoreIterator it = container.begin(MWWorld::ContainerStore::Type_Miscellaneous);
                 it != container.end(); ++it)
            {
                const std::string& id = it->getCellRef().mRefID;
                if (id.size() >= soulgemFilter.size()
                        && id.substr(0,soulgemFilter.size()) == soulgemFilter)
                {
                    float thisGemCapacity = it->get<ESM::Miscellaneous>()->mBase->mData.mValue * fSoulgemMult;
                    if (thisGemCapacity >= creatureSoulValue && thisGemCapacity < gemCapacity
                            && it->getCellRef().mSoul.empty())
                    {
                        gem = it;
                        gemCapacity = thisGemCapacity;
                    }
                }
            }

            if (gem == container.end())
                return;

            // Set the soul on just one of the gems, not the whole stack
            gem->getContainerStore()->unstack(*gem, caster);
            gem->getCellRef().mSoul = mCreature.getCellRef().mRefID;

            if (caster.getRefData().getHandle() == "player")
                MWBase::Environment::get().getWindowManager()->messageBox("#{sSoultrapSuccess}");
        }
    };

    void Actors::updateActor (const MWWorld::Ptr& ptr, float duration)
    {
        // magic effects
        adjustMagicEffects (ptr);
        if (ptr.getClass().getCreatureStats(ptr).needToRecalcDynamicStats())
            calculateDynamicStats (ptr);
        calculateCreatureStatModifiers (ptr, duration);

        // AI
        if(MWBase::Environment::get().getMechanicsManager()->isAIActive())
        {
            CreatureStats& creatureStats =  MWWorld::Class::get (ptr).getCreatureStats (ptr);
            //engage combat or not?
            MWWorld::Ptr player = MWBase::Environment::get().getWorld()->getPlayerPtr();
            if(ptr != player && !creatureStats.isHostile())
            {
                ESM::Position playerpos = player.getRefData().getPosition();
                ESM::Position actorpos = ptr.getRefData().getPosition();
                float d = sqrt((actorpos.pos[0] - playerpos.pos[0])*(actorpos.pos[0] - playerpos.pos[0])
                    +(actorpos.pos[1] - playerpos.pos[1])*(actorpos.pos[1] - playerpos.pos[1])
                    +(actorpos.pos[2] - playerpos.pos[2])*(actorpos.pos[2] - playerpos.pos[2]));
                float fight = ptr.getClass().getCreatureStats(ptr).getAiSetting(CreatureStats::AI_Fight).getModified();
                float disp = 100; //creatures don't have disposition, so set it to 100 by default
                if(ptr.getTypeName() == typeid(ESM::NPC).name())
                {
                    disp = MWBase::Environment::get().getMechanicsManager()->getDerivedDisposition(ptr);
                }
                bool LOS = MWBase::Environment::get().getWorld()->getLOS(ptr,player)
                        && MWBase::Environment::get().getMechanicsManager()->awarenessCheck(player, ptr);
                if(  ( (fight == 100 )
                    || (fight >= 95 && d <= 3000)
                    || (fight >= 90 && d <= 2000)
                    || (fight >= 80 && d <= 1000)
                    || (fight >= 80 && disp <= 40)
                    || (fight >= 70 && disp <= 35 && d <= 1000)
                    || (fight >= 60 && disp <= 30 && d <= 1000)
                    || (fight >= 50 && disp == 0)
                    || (fight >= 40 && disp <= 10 && d <= 500) )
                    && LOS
                    )
                {
                    creatureStats.getAiSequence().stack(AiCombat("player"));
                    creatureStats.setHostile(true);
                }
            }

            creatureStats.getAiSequence().execute (ptr,duration);
        }

        // fatigue restoration
        calculateRestoration(ptr, duration);
    }

    void Actors::updateNpc (const MWWorld::Ptr& ptr, float duration, bool paused)
    {
        if(!paused)
        {
            updateDrowning(ptr, duration);
            calculateNpcStatModifiers(ptr);
            updateEquippedLight(ptr, duration);
        }
    }

    void Actors::adjustMagicEffects (const MWWorld::Ptr& creature)
    {
        CreatureStats& creatureStats =  MWWorld::Class::get (creature).getCreatureStats (creature);

        MagicEffects now = creatureStats.getSpells().getMagicEffects();

        if (creature.getTypeName()==typeid (ESM::NPC).name())
        {
            MWWorld::InventoryStore& store = MWWorld::Class::get (creature).getInventoryStore (creature);
            now += store.getMagicEffects();
        }

        now += creatureStats.getActiveSpells().getMagicEffects();

        //MagicEffects diff = MagicEffects::diff (creatureStats.getMagicEffects(), now);

        creatureStats.setMagicEffects(now);

        // TODO apply diff to other stats
    }

    void Actors::calculateDynamicStats (const MWWorld::Ptr& ptr)
    {
        CreatureStats& creatureStats = MWWorld::Class::get (ptr).getCreatureStats (ptr);

        int strength     = creatureStats.getAttribute(ESM::Attribute::Strength).getBase();
        int intelligence = creatureStats.getAttribute(ESM::Attribute::Intelligence).getBase();
        int willpower    = creatureStats.getAttribute(ESM::Attribute::Willpower).getBase();
        int agility      = creatureStats.getAttribute(ESM::Attribute::Agility).getBase();
        int endurance    = creatureStats.getAttribute(ESM::Attribute::Endurance).getBase();

        double magickaFactor =
            creatureStats.getMagicEffects().get (EffectKey (ESM::MagicEffect::FortifyMaximumMagicka)).mMagnitude * 0.1 + 0.5;

        DynamicStat<float> magicka = creatureStats.getMagicka();
        float diff = (static_cast<int>(intelligence + magickaFactor*intelligence)) - magicka.getBase();
        magicka.modify(diff);
        creatureStats.setMagicka(magicka);

        DynamicStat<float> fatigue = creatureStats.getFatigue();
        diff = (strength+willpower+agility+endurance) - fatigue.getBase();
        fatigue.modify(diff);
        creatureStats.setFatigue(fatigue);
    }

    void Actors::calculateRestoration (const MWWorld::Ptr& ptr, float duration)
    {
        if (ptr.getClass().getCreatureStats(ptr).isDead())
            return;
        CreatureStats& stats = MWWorld::Class::get (ptr).getCreatureStats (ptr);
        const MWWorld::Store<ESM::GameSetting>& settings = MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>();

        int endurance = stats.getAttribute (ESM::Attribute::Endurance).getModified ();

        float capacity = MWWorld::Class::get(ptr).getCapacity(ptr);
        float encumbrance = MWWorld::Class::get(ptr).getEncumbrance(ptr);
        float normalizedEncumbrance = (capacity == 0 ? 1 : encumbrance/capacity);
        if (normalizedEncumbrance > 1)
            normalizedEncumbrance = 1;

        if (duration == 3600)
        {
            // the actor is sleeping, restore health and magicka

            bool stunted = stats.getMagicEffects ().get(ESM::MagicEffect::StuntedMagicka).mMagnitude > 0;

            DynamicStat<float> health = stats.getHealth();
            health.setCurrent (health.getCurrent() + 0.1 * endurance);
            stats.setHealth (health);

            if (!stunted)
            {
                float fRestMagicMult = settings.find("fRestMagicMult")->getFloat ();

                DynamicStat<float> magicka = stats.getMagicka();
                magicka.setCurrent (magicka.getCurrent()
                    + fRestMagicMult * stats.getAttribute(ESM::Attribute::Intelligence).getModified());
                stats.setMagicka (magicka);
            }
        }

        // restore fatigue

        float fFatigueReturnBase = settings.find("fFatigueReturnBase")->getFloat ();
        float fFatigueReturnMult = settings.find("fFatigueReturnMult")->getFloat ();
        float fEndFatigueMult = settings.find("fEndFatigueMult")->getFloat ();

        float x = fFatigueReturnBase + fFatigueReturnMult * (1 - normalizedEncumbrance);
        x *= fEndFatigueMult * endurance;

        DynamicStat<float> fatigue = stats.getFatigue();
        fatigue.setCurrent (fatigue.getCurrent() + duration * x);
        stats.setFatigue (fatigue);
    }

    void Actors::calculateCreatureStatModifiers (const MWWorld::Ptr& ptr, float duration)
    {
        CreatureStats &creatureStats = MWWorld::Class::get(ptr).getCreatureStats(ptr);
        const MagicEffects &effects = creatureStats.getMagicEffects();

        // attributes
        for(int i = 0;i < ESM::Attribute::Length;++i)
        {
            AttributeValue stat = creatureStats.getAttribute(i);
            stat.setModifier(effects.get(EffectKey(ESM::MagicEffect::FortifyAttribute, i)).mMagnitude -
                             effects.get(EffectKey(ESM::MagicEffect::DrainAttribute, i)).mMagnitude -
                             effects.get(EffectKey(ESM::MagicEffect::AbsorbAttribute, i)).mMagnitude);

            creatureStats.setAttribute(i, stat);
        }

        // dynamic stats
        for(int i = 0;i < 3;++i)
        {
            DynamicStat<float> stat = creatureStats.getDynamic(i);
            stat.setModifier(effects.get(ESM::MagicEffect::FortifyHealth+i).mMagnitude -
                             effects.get(ESM::MagicEffect::DrainHealth+i).mMagnitude);


            float currentDiff = creatureStats.getMagicEffects().get(ESM::MagicEffect::RestoreHealth+i).mMagnitude
                    - creatureStats.getMagicEffects().get(ESM::MagicEffect::DamageHealth+i).mMagnitude
                    - creatureStats.getMagicEffects().get(ESM::MagicEffect::AbsorbHealth+i).mMagnitude;
            stat.setCurrent(stat.getCurrent() + currentDiff * duration);

            creatureStats.setDynamic(i, stat);
        }

        // AI setting modifiers
        int creature = !ptr.getClass().isNpc();
        if (creature && ptr.get<ESM::Creature>()->mBase->mData.mType == ESM::Creature::Humanoid)
            creature = false;
        // Note: the Creature variants only work on normal creatures, not on daedra or undead creatures.
        if (!creature || ptr.get<ESM::Creature>()->mBase->mData.mType == ESM::Creature::Creatures)
        {
            Stat<int> stat = creatureStats.getAiSetting(CreatureStats::AI_Fight);
            stat.setModifier(creatureStats.getMagicEffects().get(ESM::MagicEffect::FrenzyHumanoid+creature).mMagnitude
                - creatureStats.getMagicEffects().get(ESM::MagicEffect::CalmHumanoid+creature).mMagnitude);
            creatureStats.setAiSetting(CreatureStats::AI_Fight, stat);

            stat = creatureStats.getAiSetting(CreatureStats::AI_Flee);
            stat.setModifier(creatureStats.getMagicEffects().get(ESM::MagicEffect::DemoralizeHumanoid+creature).mMagnitude
                - creatureStats.getMagicEffects().get(ESM::MagicEffect::RallyHumanoid+creature).mMagnitude);
            creatureStats.setAiSetting(CreatureStats::AI_Flee, stat);
        }
        if (creature && ptr.get<ESM::Creature>()->mBase->mData.mType == ESM::Creature::Undead)
        {
            Stat<int> stat = creatureStats.getAiSetting(CreatureStats::AI_Flee);
            stat.setModifier(creatureStats.getMagicEffects().get(ESM::MagicEffect::TurnUndead).mMagnitude);
            creatureStats.setAiSetting(CreatureStats::AI_Flee, stat);
        }

        // Apply disintegration (reduces item health)
        float disintegrateWeapon = effects.get(ESM::MagicEffect::DisintegrateWeapon).mMagnitude;
        if (disintegrateWeapon > 0)
            disintegrateSlot(ptr, MWWorld::InventoryStore::Slot_CarriedRight, disintegrateWeapon*duration);
        float disintegrateArmor = effects.get(ESM::MagicEffect::DisintegrateArmor).mMagnitude;
        if (disintegrateArmor > 0)
        {
            // According to UESP
            int priorities[] = {
                MWWorld::InventoryStore::Slot_CarriedLeft,
                MWWorld::InventoryStore::Slot_Cuirass,
                MWWorld::InventoryStore::Slot_LeftPauldron,
                MWWorld::InventoryStore::Slot_RightPauldron,
                MWWorld::InventoryStore::Slot_LeftGauntlet,
                MWWorld::InventoryStore::Slot_RightGauntlet,
                MWWorld::InventoryStore::Slot_Helmet,
                MWWorld::InventoryStore::Slot_Greaves,
                MWWorld::InventoryStore::Slot_Boots
            };

            for (unsigned int i=0; i<sizeof(priorities)/sizeof(int); ++i)
            {
                if (disintegrateSlot(ptr, priorities[i], disintegrateArmor*duration))
                    break;
            }
        }

        // Apply damage ticks
        int damageEffects[] = {
            ESM::MagicEffect::FireDamage, ESM::MagicEffect::ShockDamage, ESM::MagicEffect::FrostDamage, ESM::MagicEffect::Poison,
            ESM::MagicEffect::SunDamage
        };

        DynamicStat<float> health = creatureStats.getHealth();
        for (unsigned int i=0; i<sizeof(damageEffects)/sizeof(int); ++i)
        {
            float magnitude = creatureStats.getMagicEffects().get(damageEffects[i]).mMagnitude;

            if (damageEffects[i] == ESM::MagicEffect::SunDamage)
            {
                // isInCell shouldn't be needed, but updateActor called during game start
                if (!ptr.isInCell() || !ptr.getCell()->isExterior())
                    continue;
                float time = MWBase::Environment::get().getWorld()->getTimeStamp().getHour();
                float timeDiff = std::min(7.f, std::max(0.f, std::abs(time - 13)));
                float damageScale = 1.f - timeDiff / 7.f;
                // When cloudy, the sun damage effect is halved
                static float fMagicSunBlockedMult = MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>().find(
                            "fMagicSunBlockedMult")->getFloat();

                int weather = MWBase::Environment::get().getWorld()->getCurrentWeather();
                if (weather > 1)
                    damageScale *= fMagicSunBlockedMult;
                health.setCurrent(health.getCurrent() - magnitude * duration * damageScale);
            }
            else
                health.setCurrent(health.getCurrent() - magnitude * duration);

        }
        creatureStats.setHealth(health);

        // TODO: dirty flag for magic effects to avoid some unnecessary work below?

        // Update bound effects
        static std::map<int, std::string> boundItemsMap;
        if (boundItemsMap.empty())
        {
            boundItemsMap[ESM::MagicEffect::BoundBattleAxe] = "sMagicBoundBattleAxeID";
            boundItemsMap[ESM::MagicEffect::BoundBoots] = "sMagicBoundBootsID";
            boundItemsMap[ESM::MagicEffect::BoundCuirass] = "sMagicBoundCuirassID";
            boundItemsMap[ESM::MagicEffect::BoundDagger] = "sMagicBoundDaggerID";
            boundItemsMap[ESM::MagicEffect::BoundGloves] = "sMagicBoundLeftGauntletID"; // Note: needs RightGauntlet variant too (see below)
            boundItemsMap[ESM::MagicEffect::BoundHelm] = "sMagicBoundHelmID";
            boundItemsMap[ESM::MagicEffect::BoundLongbow] = "sMagicBoundLongbowID";
            boundItemsMap[ESM::MagicEffect::BoundLongsword] = "sMagicBoundLongswordID";
            boundItemsMap[ESM::MagicEffect::BoundMace] = "sMagicBoundMaceID";
            boundItemsMap[ESM::MagicEffect::BoundShield] = "sMagicBoundShieldID";
            boundItemsMap[ESM::MagicEffect::BoundSpear] = "sMagicBoundSpearID";
        }

        for (std::map<int, std::string>::iterator it = boundItemsMap.begin(); it != boundItemsMap.end(); ++it)
        {
            bool found = creatureStats.mBoundItems.find(it->first) != creatureStats.mBoundItems.end();
            int magnitude = creatureStats.getMagicEffects().get(it->first).mMagnitude;
            if (found != (magnitude > 0))
            {
                std::string itemGmst = it->second;
                std::string item = MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>().find(
                            itemGmst)->getString();
                if (it->first == ESM::MagicEffect::BoundGloves)
                {
                    adjustBoundItem("sMagicBoundLeftGauntletID", magnitude > 0, ptr);
                    adjustBoundItem("sMagicBoundRightGauntletID", magnitude > 0, ptr);
                }
                else
                    adjustBoundItem(item, magnitude > 0, ptr);

                if (magnitude > 0)
                    creatureStats.mBoundItems.insert(it->first);
                else
                    creatureStats.mBoundItems.erase(it->first);
            }
        }

        // Update summon effects
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

        for (std::map<int, std::string>::iterator it = summonMap.begin(); it != summonMap.end(); ++it)
        {
            bool found = creatureStats.mSummonedCreatures.find(it->first) != creatureStats.mSummonedCreatures.end();
            int magnitude = creatureStats.getMagicEffects().get(it->first).mMagnitude;
            if (found != (magnitude > 0))
            {
                if (magnitude > 0)
                {
                    ESM::Position ipos = ptr.getRefData().getPosition();
                    Ogre::Vector3 pos(ipos.pos[0],ipos.pos[1],ipos.pos[2]);
                    Ogre::Quaternion rot(Ogre::Radian(-ipos.rot[2]), Ogre::Vector3::UNIT_Z);
                    const float distance = 50;
                    pos = pos + distance*rot.yAxis();
                    ipos.pos[0] = pos.x;
                    ipos.pos[1] = pos.y;
                    ipos.pos[2] = pos.z;
                    ipos.rot[0] = 0;
                    ipos.rot[1] = 0;
                    ipos.rot[2] = 0;

                    std::string creatureID =
                            MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>().find(it->second)->getString();

                    if (!creatureID.empty())
                    {
                        MWWorld::CellStore* store = ptr.getCell();
                        MWWorld::ManualRef ref(MWBase::Environment::get().getWorld()->getStore(), creatureID, 1);
                        ref.getPtr().getCellRef().mPos = ipos;

                        // TODO: Add AI to follow player and fight for him
                        // TODO: VFX_SummonStart, VFX_SummonEnd
                        creatureStats.mSummonedCreatures.insert(std::make_pair(it->first,
                            MWBase::Environment::get().getWorld()->safePlaceObject(ref.getPtr(),*store,ipos).getRefData().getHandle()));
                    }
                }
                else
                {
                    std::string handle = creatureStats.mSummonedCreatures[it->first];
                    // TODO: Show death animation before deleting? We shouldn't allow looting the corpse while the animation
                    // plays though, which is a rather lame exploit in vanilla.
                    MWWorld::Ptr ptr = MWBase::Environment::get().getWorld()->searchPtrViaHandle(handle);
                    if (!ptr.isEmpty())
                    {
                        MWBase::Environment::get().getWorld()->deleteObject(ptr);
                        creatureStats.mSummonedCreatures.erase(it->first);
                    }
                }
            }
        }
    }

    void Actors::calculateNpcStatModifiers (const MWWorld::Ptr& ptr)
    {
        NpcStats &npcStats = MWWorld::Class::get(ptr).getNpcStats(ptr);
        const MagicEffects &effects = npcStats.getMagicEffects();

        // skills
        for(int i = 0;i < ESM::Skill::Length;++i)
        {
            SkillValue& skill = npcStats.getSkill(i);
            skill.setModifier(effects.get(EffectKey(ESM::MagicEffect::FortifySkill, i)).mMagnitude -
                             effects.get(EffectKey(ESM::MagicEffect::DrainSkill, i)).mMagnitude -
                             effects.get(EffectKey(ESM::MagicEffect::AbsorbSkill, i)).mMagnitude);
        }
    }

    void Actors::updateDrowning(const MWWorld::Ptr& ptr, float duration)
    {
        MWBase::World *world = MWBase::Environment::get().getWorld();
        NpcStats &stats = ptr.getClass().getNpcStats(ptr);
        if(world->isSubmerged(ptr) &&
           stats.getMagicEffects().get(ESM::MagicEffect::WaterBreathing).mMagnitude == 0)
        {
            float timeLeft = 0.0f;
            if(stats.getFatigue().getCurrent() == 0)
                stats.setTimeToStartDrowning(0);
            else
            {
                timeLeft = stats.getTimeToStartDrowning() - duration;
                if(timeLeft < 0.0f)
                    timeLeft = 0.0f;
                stats.setTimeToStartDrowning(timeLeft);
            }
            if(timeLeft == 0.0f)
            {
                // If drowning, apply 3 points of damage per second
                static const float fSuffocationDamage = world->getStore().get<ESM::GameSetting>().find("fSuffocationDamage")->getFloat();
                ptr.getClass().setActorHealth(ptr, stats.getHealth().getCurrent() - fSuffocationDamage*duration);

                // Play a drowning sound as necessary for the player
                if(ptr == world->getPlayerPtr())
                {
                    MWBase::SoundManager *sndmgr = MWBase::Environment::get().getSoundManager();
                    if(!sndmgr->getSoundPlaying(MWWorld::Ptr(), "drown"))
                        sndmgr->playSound("drown", 1.0f, 1.0f);
                }
            }
        }
        else
        {
            static const float fHoldBreathTime = world->getStore().get<ESM::GameSetting>().find("fHoldBreathTime")->getFloat();
            stats.setTimeToStartDrowning(fHoldBreathTime);
        }
    }

    void Actors::updateEquippedLight (const MWWorld::Ptr& ptr, float duration)
    {
        bool isPlayer = ptr.getRefData().getHandle()=="player";

        MWWorld::InventoryStore &inventoryStore = MWWorld::Class::get(ptr).getInventoryStore(ptr);
        MWWorld::ContainerStoreIterator heldIter =
                inventoryStore.getSlot(MWWorld::InventoryStore::Slot_CarriedLeft);
        /**
         * Automatically equip NPCs torches at night and unequip them at day
         */
        if (!isPlayer)
        {
            MWWorld::ContainerStoreIterator torch = inventoryStore.end();
            for (MWWorld::ContainerStoreIterator it = inventoryStore.begin(); it != inventoryStore.end(); ++it)
            {
                if (it->getTypeName() == typeid(ESM::Light).name())
                {
                    torch = it;
                    break;
                }
            }

            if (MWBase::Environment::get().getWorld()->isDark())
            {
                if (torch != inventoryStore.end())
                {
                    if (!MWWorld::Class::get (ptr).getCreatureStats (ptr).isHostile())
                    {
                        // For non-hostile NPCs, unequip whatever is in the left slot in favor of a light.
                        if (heldIter != inventoryStore.end() && heldIter->getTypeName() != typeid(ESM::Light).name())
                            inventoryStore.unequipItem(*heldIter, ptr);

                        // Also unequip twohanded weapons which conflict with anything in CarriedLeft
                        if (torch->getClass().canBeEquipped(*torch, ptr).first == 3)
                            inventoryStore.unequipSlot(MWWorld::InventoryStore::Slot_CarriedRight, ptr);
                    }

                    heldIter = inventoryStore.getSlot(MWWorld::InventoryStore::Slot_CarriedLeft);

                    // If we have a torch and can equip it (left slot free, no
                    // twohanded weapon in right slot), then equip it now.
                    if (heldIter == inventoryStore.end()
                            && torch->getClass().canBeEquipped(*torch, ptr).first == 1)
                    {
                        inventoryStore.equip(MWWorld::InventoryStore::Slot_CarriedLeft, torch, ptr);
                    }
                }
            }
            else
            {
                if (heldIter != inventoryStore.end() && heldIter->getTypeName() == typeid(ESM::Light).name())
                {
                    // At day, unequip lights and auto equip shields or other suitable items
                    // (Note: autoEquip will ignore lights)
                    inventoryStore.autoEquip(ptr);
                }
            }
        }

        heldIter = inventoryStore.getSlot(MWWorld::InventoryStore::Slot_CarriedLeft);

        //If holding a light...
        if(heldIter.getType() == MWWorld::ContainerStore::Type_Light)
        {
            // Use time from the player's light
            if(isPlayer)
            {
                float timeRemaining = heldIter->getClass().getRemainingUsageTime(*heldIter);

                // -1 is infinite light source. Other negative values are treated as 0.
                if(timeRemaining != -1.0f)
                {
                    timeRemaining -= duration;

                    if(timeRemaining > 0.0f)
                        heldIter->getClass().setRemainingUsageTime(*heldIter, timeRemaining);
                    else
                    {
                        inventoryStore.remove(*heldIter, 1, ptr); // remove it
                        return;
                    }
                }
            }

            // Both NPC and player lights extinguish in water.
            if(MWBase::Environment::get().getWorld()->isSwimming(ptr))
            {
                inventoryStore.remove(*heldIter, 1, ptr); // remove it

                // ...But, only the player makes a sound.
                if(isPlayer)
                    MWBase::Environment::get().getSoundManager()->playSound("torch out",
                            1.0, 1.0, MWBase::SoundManager::Play_TypeSfx, MWBase::SoundManager::Play_NoEnv);
            }
        }
    }

    Actors::Actors() {}

    Actors::~Actors()
    {
      PtrControllerMap::iterator it(mActors.begin());
      for (; it != mActors.end(); ++it)
      {
        delete it->second;
        it->second = NULL;
      }
    }

    void Actors::addActor (const MWWorld::Ptr& ptr)
    {
        // erase previous death events since we are currently only tracking them while in an active cell
        MWWorld::Class::get(ptr).getCreatureStats(ptr).clearHasDied();

        removeActor(ptr);

        MWRender::Animation *anim = MWBase::Environment::get().getWorld()->getAnimation(ptr);
        mActors.insert(std::make_pair(ptr, new CharacterController(ptr, anim)));
    }

    void Actors::removeActor (const MWWorld::Ptr& ptr)
    {
        PtrControllerMap::iterator iter = mActors.find(ptr);
        if(iter != mActors.end())
        {
            delete iter->second;
            mActors.erase(iter);
        }
    }

    void Actors::updateActor(const MWWorld::Ptr &old, const MWWorld::Ptr &ptr)
    {
        PtrControllerMap::iterator iter = mActors.find(old);
        if(iter != mActors.end())
        {
            CharacterController *ctrl = iter->second;
            mActors.erase(iter);

            ctrl->updatePtr(ptr);
            mActors.insert(std::make_pair(ptr, ctrl));
        }
    }

    void Actors::dropActors (const MWWorld::Ptr::CellStore *cellStore, const MWWorld::Ptr& ignore)
    {
        PtrControllerMap::iterator iter = mActors.begin();
        while(iter != mActors.end())
        {
            if(iter->first.getCell()==cellStore && iter->first != ignore)
            {
                delete iter->second;
                mActors.erase(iter++);
            }
            else
                ++iter;
        }
    }

    void Actors::update (float duration, bool paused)
    {
        if (!paused)
        {
            for(PtrControllerMap::iterator iter(mActors.begin());iter != mActors.end();iter++)
            {
                const MWWorld::Class &cls = MWWorld::Class::get(iter->first);
                CreatureStats &stats = cls.getCreatureStats(iter->first);

                stats.setLastHitObject(std::string());
                if(!stats.isDead())
                {
                    if(iter->second->isDead())
                        iter->second->resurrect();

                    updateActor(iter->first, duration);
                    if(iter->first.getTypeName() == typeid(ESM::NPC).name())
                        updateNpc(iter->first, duration, paused);

                    if(!stats.isDead())
                        continue;
                }

                // If it's the player and God Mode is turned on, keep it alive
                if(iter->first.getRefData().getHandle()=="player" && 
                    MWBase::Environment::get().getWorld()->getGodModeState())
                {
                    MWMechanics::DynamicStat<float> stat(stats.getHealth());

                    if(stat.getModified()<1)
                    {
                        stat.setModified(1, 0);
                        stats.setHealth(stat);
                    }

                    stats.resurrect();
                    continue;
                }

                if(iter->second->isDead())
                    continue;

                iter->second->kill();

                // Apply soultrap
                if (iter->first.getTypeName() == typeid(ESM::Creature).name())
                {
                    SoulTrap soulTrap (iter->first);
                    stats.getActiveSpells().visitEffectSources(soulTrap);
                }

                // Reset magic effects and recalculate derived effects
                // One case where we need this is to make sure bound items are removed upon death
                stats.setMagicEffects(MWMechanics::MagicEffects());
                calculateCreatureStatModifiers(iter->first, 0);

                // Make sure spell effects with CasterLinked flag are removed
                for(PtrControllerMap::iterator iter2(mActors.begin());iter2 != mActors.end();++iter2)
                {
                    MWMechanics::ActiveSpells& spells = iter2->first.getClass().getCreatureStats(iter2->first).getActiveSpells();
                    spells.purge(iter->first.getRefData().getHandle());
                }

                ++mDeathCount[cls.getId(iter->first)];

                if(cls.isEssential(iter->first))
                    MWBase::Environment::get().getWindowManager()->messageBox("#{sKilledEssential}");

            }
        }

        if(!paused)
        {
            // Note: we need to do this before any of the animations are updated.
            // Reaching the text keys may trigger Hit / Spellcast (and as such, particles),
            // so updating VFX immediately after that would just remove the particle effects instantly.
            // There needs to be a magic effect update in between.
            for(PtrControllerMap::iterator iter(mActors.begin());iter != mActors.end();++iter)
                iter->second->updateContinuousVfx();

            for(PtrControllerMap::iterator iter(mActors.begin());iter != mActors.end();++iter)
            {
                if (iter->first.getClass().getCreatureStats(iter->first).getMagicEffects().get(
                            ESM::MagicEffect::Paralyze).mMagnitude > 0)
                    iter->second->skipAnim();
                iter->second->update(duration);
            }
        }
    }
    void Actors::restoreDynamicStats()
    {
        for(PtrControllerMap::iterator iter(mActors.begin());iter != mActors.end();++iter)
            calculateRestoration(iter->first, 3600);
    }

    int Actors::countDeaths (const std::string& id) const
    {
        std::map<std::string, int>::const_iterator iter = mDeathCount.find(id);
        if(iter != mDeathCount.end())
            return iter->second;
        return 0;
    }

    void Actors::forceStateUpdate(const MWWorld::Ptr & ptr)
    {
        PtrControllerMap::iterator iter = mActors.find(ptr);
        if(iter != mActors.end())
            iter->second->forceStateUpdate();
    }

    void Actors::playAnimationGroup(const MWWorld::Ptr& ptr, const std::string& groupName, int mode, int number)
    {
        PtrControllerMap::iterator iter = mActors.find(ptr);
        if(iter != mActors.end())
            iter->second->playGroup(groupName, mode, number);
    }
    void Actors::skipAnimation(const MWWorld::Ptr& ptr)
    {
        PtrControllerMap::iterator iter = mActors.find(ptr);
        if(iter != mActors.end())
            iter->second->skipAnim();
    }

    bool Actors::checkAnimationPlaying(const MWWorld::Ptr& ptr, const std::string& groupName)
    {
        PtrControllerMap::iterator iter = mActors.find(ptr);
        if(iter != mActors.end())
            return iter->second->isAnimPlaying(groupName);
        return false;
    }
}


#include "actors.hpp"

#include <typeinfo>

#include <OgreVector3.h>

#include <components/esm/loadnpc.hpp>

#include "../mwworld/esmstore.hpp"

#include "../mwworld/class.hpp"
#include "../mwworld/inventorystore.hpp"
#include "../mwworld/manualref.hpp"
#include "../mwworld/actionequip.hpp"
#include "../mwworld/player.hpp"

#include "../mwbase/world.hpp"
#include "../mwbase/environment.hpp"
#include "../mwbase/windowmanager.hpp"
#include "../mwbase/soundmanager.hpp"

#include "../mwrender/animation.hpp"

#include "npcstats.hpp"
#include "creaturestats.hpp"
#include "movement.hpp"
#include "character.hpp"

#include "../mwbase/environment.hpp"
#include "../mwbase/mechanicsmanager.hpp"

#include "aicombat.hpp"
#include "aifollow.hpp"
#include "aipursue.hpp"

namespace
{

void adjustBoundItem (const std::string& item, bool bound, const MWWorld::Ptr& actor)
{
    if (bound)
    {
        if (actor.getClass().getContainerStore(actor).count(item) == 0)
        {
            MWWorld::Ptr newPtr = *actor.getClass().getContainerStore(actor).add(item, 1, actor);
            MWWorld::ActionEquip action(newPtr);
            action.execute(actor);
        }
    }
    else
    {
        actor.getClass().getContainerStore(actor).remove(item, 1, actor);
    }
}

bool disintegrateSlot (MWWorld::Ptr ptr, int slot, float disintegrate)
{
    if (ptr.getClass().hasInventoryStore(ptr))
    {
        MWWorld::InventoryStore& inv = ptr.getClass().getInventoryStore(ptr);
        MWWorld::ContainerStoreIterator item =
                inv.getSlot(slot);
        if (item != inv.end())
        {
            if (!item->getClass().hasItemHealth(*item))
                return false;
            int charge = item->getClass().getItemHealth(*item);

            if (charge == 0)
                return false;

            charge -=
                    std::min(disintegrate,
                             static_cast<float>(charge));
            item->getCellRef().setCharge(charge);

            if (charge == 0)
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

class CheckActorCommanded : public MWMechanics::EffectSourceVisitor
{
    MWWorld::Ptr mActor;
public:
    bool mCommanded;
    CheckActorCommanded(MWWorld::Ptr actor)
        : mActor(actor)
    , mCommanded(false){}

    virtual void visit (MWMechanics::EffectKey key,
                             const std::string& sourceName, int casterActorId,
                        float magnitude, float remainingTime = -1)
    {
        MWWorld::Ptr player = MWBase::Environment::get().getWorld()->getPlayerPtr();
        if (    ((key.mId == ESM::MagicEffect::CommandHumanoid && mActor.getClass().isNpc())
                || (key.mId == ESM::MagicEffect::CommandCreature && mActor.getTypeName() == typeid(ESM::Creature).name()))
            && casterActorId == player.getClass().getCreatureStats(player).getActorId()
            && magnitude >= mActor.getClass().getCreatureStats(mActor).getLevel())
                mCommanded = true;
    }
};

void adjustCommandedActor (const MWWorld::Ptr& actor)
{
    CheckActorCommanded check(actor);
    MWMechanics::CreatureStats& stats = actor.getClass().getCreatureStats(actor);
    stats.getActiveSpells().visitEffectSources(check);

    bool hasCommandPackage = false;

    std::list<MWMechanics::AiPackage*>::const_iterator it;
    for (it = stats.getAiSequence().begin(); it != stats.getAiSequence().end(); ++it)
    {
        if ((*it)->getTypeId() == MWMechanics::AiPackage::TypeIdFollow &&
                dynamic_cast<MWMechanics::AiFollow*>(*it)->isCommanded())
        {
            hasCommandPackage = true;
            break;
        }
    }

    if (check.mCommanded && !hasCommandPackage)
    {
        MWMechanics::AiFollow package("player", true);
        stats.getAiSequence().stack(package, actor);
    }
    else if (!check.mCommanded && hasCommandPackage)
    {
        stats.getAiSequence().erase(it);
    }
}

void getRestorationPerHourOfSleep (const MWWorld::Ptr& ptr, float& health, float& magicka)
{
    MWMechanics::CreatureStats& stats = ptr.getClass().getCreatureStats (ptr);
    const MWWorld::Store<ESM::GameSetting>& settings = MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>();

    bool stunted = stats.getMagicEffects ().get(ESM::MagicEffect::StuntedMagicka).getMagnitude() > 0;
    int endurance = stats.getAttribute (ESM::Attribute::Endurance).getModified ();

    health = 0.1 * endurance;

    magicka = 0;
    if (!stunted)
    {
        float fRestMagicMult = settings.find("fRestMagicMult")->getFloat ();
        magicka = fRestMagicMult * stats.getAttribute(ESM::Attribute::Intelligence).getModified();
    }
}

void cleanupSummonedCreature (MWMechanics::CreatureStats& casterStats, int creatureActorId)
{
    MWWorld::Ptr ptr = MWBase::Environment::get().getWorld()->searchPtrViaActorId(creatureActorId);
    if (!ptr.isEmpty())
    {
        // TODO: Show death animation before deleting? We shouldn't allow looting the corpse while the animation
        // plays though, which is a rather lame exploit in vanilla.
        MWBase::Environment::get().getWorld()->deleteObject(ptr);

        const ESM::Static* fx = MWBase::Environment::get().getWorld()->getStore().get<ESM::Static>()
                .search("VFX_Summon_End");
        if (fx)
            MWBase::Environment::get().getWorld()->spawnEffect("meshes\\" + fx->mModel,
                "", Ogre::Vector3(ptr.getRefData().getPosition().pos));
    }
    else
    {
        // We didn't find the creature. It's probably in an inactive cell.
        // Add to graveyard so we can delete it when the cell becomes active.
        std::vector<int>& graveyard = casterStats.getSummonedCreatureGraveyard();
        graveyard.push_back(creatureActorId);
    }
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
                                 const std::string& sourceName, int casterActorId,
                            float magnitude, float remainingTime = -1)
        {
            if (key.mId != ESM::MagicEffect::Soultrap)
                return;
            if (magnitude <= 0)
                return;

            MWBase::World* world = MWBase::Environment::get().getWorld();

            MWWorld::Ptr caster = world->searchPtrViaActorId(casterActorId);
            if (caster.isEmpty() || !caster.getClass().isActor())
                return;

            static const float fSoulgemMult = world->getStore().get<ESM::GameSetting>().find("fSoulgemMult")->getFloat();

            float creatureSoulValue = mCreature.get<ESM::Creature>()->mBase->mData.mSoul;
            if (creatureSoulValue == 0)
                return;

            // Use the smallest soulgem that is large enough to hold the soul
            MWWorld::ContainerStore& container = caster.getClass().getContainerStore(caster);
            MWWorld::ContainerStoreIterator gem = container.end();
            float gemCapacity = std::numeric_limits<float>::max();
            std::string soulgemFilter = "misc_soulgem"; // no other way to check for soulgems? :/
            for (MWWorld::ContainerStoreIterator it = container.begin(MWWorld::ContainerStore::Type_Miscellaneous);
                 it != container.end(); ++it)
            {
                const std::string& id = it->getCellRef().getRefId();
                if (id.size() >= soulgemFilter.size()
                        && id.substr(0,soulgemFilter.size()) == soulgemFilter)
                {
                    float thisGemCapacity = it->get<ESM::Miscellaneous>()->mBase->mData.mValue * fSoulgemMult;
                    if (thisGemCapacity >= creatureSoulValue && thisGemCapacity < gemCapacity
                            && it->getCellRef().getSoul().empty())
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
            gem->getCellRef().setSoul(mCreature.getCellRef().getRefId());

            if (caster.getRefData().getHandle() == "player")
                MWBase::Environment::get().getWindowManager()->messageBox("#{sSoultrapSuccess}");

            const ESM::Static* fx = MWBase::Environment::get().getWorld()->getStore().get<ESM::Static>()
                    .search("VFX_Soul_Trap");
            if (fx)
                MWBase::Environment::get().getWorld()->spawnEffect("meshes\\" + fx->mModel,
                    "", Ogre::Vector3(mCreature.getRefData().getPosition().pos));

            MWBase::Environment::get().getSoundManager()->playSound3D(mCreature, "conjuration hit", 1.f, 1.f,
                                                                      MWBase::SoundManager::Play_TypeSfx, MWBase::SoundManager::Play_NoTrack);
        }
    };

    void Actors::updateActor (const MWWorld::Ptr& ptr, float duration)
    {
        // magic effects
        adjustMagicEffects (ptr);
        if (ptr.getClass().getCreatureStats(ptr).needToRecalcDynamicStats())
            calculateDynamicStats (ptr);

        calculateCreatureStatModifiers (ptr, duration);
        // fatigue restoration
        calculateRestoration(ptr, duration);
    }

    void Actors::engageCombat (const MWWorld::Ptr& actor1, const MWWorld::Ptr& actor2, bool againstPlayer)
    {
        CreatureStats& creatureStats = actor1.getClass().getCreatureStats(actor1);

        if (actor2.getClass().getCreatureStats(actor2).isDead()
                || actor1.getClass().getCreatureStats(actor1).isDead())
            return;

        const ESM::Position& actor1Pos = actor1.getRefData().getPosition();
        const ESM::Position& actor2Pos = actor2.getRefData().getPosition();
        float sqrDist = Ogre::Vector3(actor1Pos.pos).squaredDistance(Ogre::Vector3(actor2Pos.pos));
        if (sqrDist > 7168*7168)
            return;

        // pure water creatures won't try to fight with the target on the ground
        // except that creature is already hostile
        if ((againstPlayer || !creatureStats.getAiSequence().isInCombat())
                && ((actor1.getClass().canSwim(actor1) && !actor1.getClass().canWalk(actor1) // pure water creature
                && !MWBase::Environment::get().getWorld()->isSwimming(actor2))
                || (!actor1.getClass().canSwim(actor1) && MWBase::Environment::get().getWorld()->isSwimming(actor2)))) // creature can't swim to target
            return;

        bool aggressive;

        if (againstPlayer)
        {
            // followers with high fight should not engage in combat with the player (e.g. bm_bear_black_summon)
            const std::list<MWWorld::Ptr>& followers = getActorsFollowing(actor2);
            if (std::find(followers.begin(), followers.end(), actor1) != followers.end())
                return;

            aggressive = MWBase::Environment::get().getMechanicsManager()->isAggressive(actor1, actor2);
        }
        else
        {
            aggressive = false;

            // Make guards fight aggressive creatures
            if (!actor1.getClass().isNpc() && actor2.getClass().isClass(actor2, "Guard"))
            {
                if (creatureStats.getAiSequence().isInCombat() && MWBase::Environment::get().getMechanicsManager()->isAggressive(actor1, actor2))
                    aggressive = true;
            }
        }

        // start combat if target actor is in combat with one of our followers
        const std::list<MWWorld::Ptr>& followers = getActorsFollowing(actor1);
        const CreatureStats& creatureStats2 = actor2.getClass().getCreatureStats(actor2);
        for (std::list<MWWorld::Ptr>::const_iterator it = followers.begin(); it != followers.end(); ++it)
        {
            // need to check both ways since player doesn't use AI packages
            if ((creatureStats2.getAiSequence().isInCombat(*it)
                    || it->getClass().getCreatureStats(*it).getAiSequence().isInCombat(actor2))
                    && !creatureStats.getAiSequence().isInCombat(*it))
                aggressive = true;
        }

        // start combat if target actor is in combat with someone we are following
        for (std::list<MWMechanics::AiPackage*>::const_iterator it = creatureStats.getAiSequence().begin(); it != creatureStats.getAiSequence().end(); ++it)
        {
            if ((*it)->getTypeId() == MWMechanics::AiPackage::TypeIdFollow)
            {
                MWWorld::Ptr followTarget = dynamic_cast<MWMechanics::AiFollow*>(*it)->getTarget();
                if (followTarget.isEmpty())
                    continue;

                if (creatureStats.getAiSequence().isInCombat(followTarget))
                    continue;

                // need to check both ways since player doesn't use AI packages
                if (creatureStats2.getAiSequence().isInCombat(followTarget)
                        || followTarget.getClass().getCreatureStats(followTarget).getAiSequence().isInCombat(actor2))
                    aggressive = true;
            }
        }

        if(aggressive)
        {
            bool LOS = MWBase::Environment::get().getWorld()->getLOS(actor1, actor2);

            if (againstPlayer) LOS &= MWBase::Environment::get().getMechanicsManager()->awarenessCheck(actor2, actor1);

            if (LOS)
            {
                MWBase::Environment::get().getMechanicsManager()->startCombat(actor1, actor2);
                if (!againstPlayer) // start combat between each other
                {
                    MWBase::Environment::get().getMechanicsManager()->startCombat(actor2, actor1);
                }
            }
        }
    }

    void Actors::updateNpc (const MWWorld::Ptr& ptr, float duration)
    {
        updateDrowning(ptr, duration);
        calculateNpcStatModifiers(ptr, duration);
        updateEquippedLight(ptr, duration);
    }

    void Actors::adjustMagicEffects (const MWWorld::Ptr& creature)
    {
        CreatureStats& creatureStats =  creature.getClass().getCreatureStats (creature);
        if (creatureStats.isDead())
            return;

        MagicEffects now = creatureStats.getSpells().getMagicEffects();

        if (creature.getTypeName()==typeid (ESM::NPC).name())
        {
            MWWorld::InventoryStore& store = creature.getClass().getInventoryStore (creature);
            now += store.getMagicEffects();
        }

        now += creatureStats.getActiveSpells().getMagicEffects();

        creatureStats.modifyMagicEffects(now);
    }

    void Actors::calculateDynamicStats (const MWWorld::Ptr& ptr)
    {
        CreatureStats& creatureStats = ptr.getClass().getCreatureStats (ptr);

        int strength     = creatureStats.getAttribute(ESM::Attribute::Strength).getModified();
        int intelligence = creatureStats.getAttribute(ESM::Attribute::Intelligence).getModified();
        int willpower    = creatureStats.getAttribute(ESM::Attribute::Willpower).getModified();
        int agility      = creatureStats.getAttribute(ESM::Attribute::Agility).getModified();
        int endurance    = creatureStats.getAttribute(ESM::Attribute::Endurance).getModified();

        float base = 1.f;
        if (ptr.getCellRef().getRefId() == "player")
            base = MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>().find("fPCbaseMagickaMult")->getFloat();
        else
            base = MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>().find("fNPCbaseMagickaMult")->getFloat();

        double magickaFactor = base +
            creatureStats.getMagicEffects().get (EffectKey (ESM::MagicEffect::FortifyMaximumMagicka)).getMagnitude() * 0.1;

        DynamicStat<float> magicka = creatureStats.getMagicka();
        float diff = (static_cast<int>(magickaFactor*intelligence)) - magicka.getBase();
        magicka.modify(diff);
        creatureStats.setMagicka(magicka);

        DynamicStat<float> fatigue = creatureStats.getFatigue();
        diff = (strength+willpower+agility+endurance) - fatigue.getBase();
        fatigue.modify(diff);
        creatureStats.setFatigue(fatigue);
    }

    void Actors::restoreDynamicStats (const MWWorld::Ptr& ptr, bool sleep)
    {
        if (ptr.getClass().getCreatureStats(ptr).isDead())
            return;

        MWMechanics::CreatureStats& stats = ptr.getClass().getCreatureStats (ptr);
        const MWWorld::Store<ESM::GameSetting>& settings = MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>();

        if (sleep)
        {
            float health, magicka;
            getRestorationPerHourOfSleep(ptr, health, magicka);

            DynamicStat<float> stat = stats.getHealth();
            stat.setCurrent(stat.getCurrent() + health);
            stats.setHealth(stat);

            stat = stats.getMagicka();
            stat.setCurrent(stat.getCurrent() + magicka);
            stats.setMagicka(stat);
        }

        int endurance = stats.getAttribute (ESM::Attribute::Endurance).getModified ();

        float normalizedEncumbrance = ptr.getClass().getNormalizedEncumbrance(ptr);
        if (normalizedEncumbrance > 1)
            normalizedEncumbrance = 1;

        // restore fatigue
        float fFatigueReturnBase = settings.find("fFatigueReturnBase")->getFloat ();
        float fFatigueReturnMult = settings.find("fFatigueReturnMult")->getFloat ();
        float fEndFatigueMult = settings.find("fEndFatigueMult")->getFloat ();

        float x = fFatigueReturnBase + fFatigueReturnMult * (1 - normalizedEncumbrance);
        x *= fEndFatigueMult * endurance;

        DynamicStat<float> fatigue = stats.getFatigue();
        fatigue.setCurrent (fatigue.getCurrent() + 3600 * x);
        stats.setFatigue (fatigue);
    }

    void Actors::calculateRestoration (const MWWorld::Ptr& ptr, float duration)
    {
        if (ptr.getClass().getCreatureStats(ptr).isDead())
            return;

        MWMechanics::CreatureStats& stats = ptr.getClass().getCreatureStats (ptr);

        int endurance = stats.getAttribute (ESM::Attribute::Endurance).getModified ();

        // restore fatigue
        const MWWorld::Store<ESM::GameSetting>& settings = MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>();
        static const float fFatigueReturnBase = settings.find("fFatigueReturnBase")->getFloat ();
        static const float fFatigueReturnMult = settings.find("fFatigueReturnMult")->getFloat ();

        float x = fFatigueReturnBase + fFatigueReturnMult * endurance;

        DynamicStat<float> fatigue = stats.getFatigue();
        fatigue.setCurrent (fatigue.getCurrent() + duration * x);
        stats.setFatigue (fatigue);
    }

    void Actors::calculateCreatureStatModifiers (const MWWorld::Ptr& ptr, float duration)
    {
        CreatureStats &creatureStats = ptr.getClass().getCreatureStats(ptr);
        const MagicEffects &effects = creatureStats.getMagicEffects();

        bool wasDead = creatureStats.isDead();

        // attributes
        for(int i = 0;i < ESM::Attribute::Length;++i)
        {
            AttributeValue stat = creatureStats.getAttribute(i);
            stat.setModifier(effects.get(EffectKey(ESM::MagicEffect::FortifyAttribute, i)).getMagnitude() -
                             effects.get(EffectKey(ESM::MagicEffect::DrainAttribute, i)).getMagnitude() -
                             effects.get(EffectKey(ESM::MagicEffect::AbsorbAttribute, i)).getMagnitude());

            stat.damage(effects.get(EffectKey(ESM::MagicEffect::DamageAttribute, i)).getMagnitude() * duration * 1.5);
            stat.restore(effects.get(EffectKey(ESM::MagicEffect::RestoreAttribute, i)).getMagnitude() * duration * 1.5);

            creatureStats.setAttribute(i, stat);
        }

        {
            Spells & spells = creatureStats.getSpells();
            for (Spells::TIterator it = spells.begin(); it != spells.end(); ++it)
            {
                if (spells.getCorprusSpells().find(it->first) != spells.getCorprusSpells().end())
                {
                    if (MWBase::Environment::get().getWorld()->getTimeStamp() >= spells.getCorprusSpells().at(it->first).mNextWorsening)
                    {
                        spells.worsenCorprus(it->first);

                        if (ptr.getRefData().getHandle() == "player")
                            MWBase::Environment::get().getWindowManager()->messageBox("#{sMagicCorprusWorsens}");
                    }
                }
            }
        }

        // dynamic stats
        for(int i = 0;i < 3;++i)
        {
            DynamicStat<float> stat = creatureStats.getDynamic(i);
            stat.setModifier(effects.get(ESM::MagicEffect::FortifyHealth+i).getMagnitude() -
                             effects.get(ESM::MagicEffect::DrainHealth+i).getMagnitude());


            float currentDiff = creatureStats.getMagicEffects().get(ESM::MagicEffect::RestoreHealth+i).getMagnitude()
                    - creatureStats.getMagicEffects().get(ESM::MagicEffect::DamageHealth+i).getMagnitude()
                    - creatureStats.getMagicEffects().get(ESM::MagicEffect::AbsorbHealth+i).getMagnitude();
            stat.setCurrent(stat.getCurrent() + currentDiff * duration, i == 2);

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
            stat.setModifier(creatureStats.getMagicEffects().get(ESM::MagicEffect::FrenzyHumanoid+creature).getMagnitude()
                - creatureStats.getMagicEffects().get(ESM::MagicEffect::CalmHumanoid+creature).getMagnitude());
            creatureStats.setAiSetting(CreatureStats::AI_Fight, stat);

            stat = creatureStats.getAiSetting(CreatureStats::AI_Flee);
            stat.setModifier(creatureStats.getMagicEffects().get(ESM::MagicEffect::DemoralizeHumanoid+creature).getMagnitude()
                - creatureStats.getMagicEffects().get(ESM::MagicEffect::RallyHumanoid+creature).getMagnitude());
            creatureStats.setAiSetting(CreatureStats::AI_Flee, stat);
        }
        if (creature && ptr.get<ESM::Creature>()->mBase->mData.mType == ESM::Creature::Undead)
        {
            Stat<int> stat = creatureStats.getAiSetting(CreatureStats::AI_Flee);
            stat.setModifier(creatureStats.getMagicEffects().get(ESM::MagicEffect::TurnUndead).getMagnitude());
            creatureStats.setAiSetting(CreatureStats::AI_Flee, stat);
        }

        // Apply disintegration (reduces item health)
        float disintegrateWeapon = effects.get(ESM::MagicEffect::DisintegrateWeapon).getMagnitude();
        if (disintegrateWeapon > 0)
            disintegrateSlot(ptr, MWWorld::InventoryStore::Slot_CarriedRight, disintegrateWeapon*duration);
        float disintegrateArmor = effects.get(ESM::MagicEffect::DisintegrateArmor).getMagnitude();
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

        bool receivedMagicDamage = false;

        if (creatureStats.getMagicEffects().get(ESM::MagicEffect::DamageHealth).getMagnitude() > 0.0f
                || creatureStats.getMagicEffects().get(ESM::MagicEffect::AbsorbHealth).getMagnitude() > 0.0f)
            receivedMagicDamage = true;

        // Apply damage ticks
        int damageEffects[] = {
            ESM::MagicEffect::FireDamage, ESM::MagicEffect::ShockDamage, ESM::MagicEffect::FrostDamage, ESM::MagicEffect::Poison,
            ESM::MagicEffect::SunDamage
        };

        DynamicStat<float> health = creatureStats.getHealth();
        for (unsigned int i=0; i<sizeof(damageEffects)/sizeof(int); ++i)
        {
            float magnitude = creatureStats.getMagicEffects().get(damageEffects[i]).getMagnitude();

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

                if (magnitude * damageScale > 0.0f)
                    receivedMagicDamage = true;
            }
            else
            {
                health.setCurrent(health.getCurrent() - magnitude * duration);

                if (magnitude > 0.0f)
                    receivedMagicDamage = true;
            }
        }

        if (receivedMagicDamage && ptr.getRefData().getHandle() == "player")
            MWBase::Environment::get().getWindowManager()->activateHitOverlay(false);

        creatureStats.setHealth(health);

        if (!wasDead && creatureStats.isDead())
        {
            // The actor was killed by a magic effect. Figure out if the player was responsible for it.
            const ActiveSpells& spells = creatureStats.getActiveSpells();
            bool killedByPlayer = false;
            bool murderedByPlayer = false;
            MWWorld::Ptr player = MWBase::Environment::get().getWorld()->getPlayerPtr();
            for (ActiveSpells::TIterator it = spells.begin(); it != spells.end(); ++it)
            {
                const ActiveSpells::ActiveSpellParams& spell = it->second;
                for (std::vector<ActiveSpells::ActiveEffect>::const_iterator effectIt = spell.mEffects.begin();
                     effectIt != spell.mEffects.end(); ++effectIt)
                {
                    int effectId = effectIt->mEffectId;
                    bool isDamageEffect = false;
                    for (unsigned int i=0; i<sizeof(damageEffects)/sizeof(int); ++i)
                    {
                        if (damageEffects[i] == effectId)
                            isDamageEffect = true;
                    }


                    if (effectId == ESM::MagicEffect::DamageHealth || effectId == ESM::MagicEffect::AbsorbHealth)
                        isDamageEffect = true;

                    MWWorld::Ptr caster = MWBase::Environment::get().getWorld()->searchPtrViaActorId(spell.mCasterActorId);
                    if (isDamageEffect && caster == player)
                    {
                        killedByPlayer = true;
                        // Simple check for who attacked first: if the player attacked first, a crimeId should be set
                        // Doesn't handle possible edge case where no one reported the assault, but in such a case,
                        // for bystanders it is not possible to tell who attacked first, anyway.
                        if (ptr.getClass().isNpc() && ptr.getClass().getNpcStats(ptr).getCrimeId() != -1
                                && ptr != player)
                            murderedByPlayer = true;
                    }
                }
            }
            if (murderedByPlayer)
                MWBase::Environment::get().getMechanicsManager()->commitCrime(player, ptr, MWBase::MechanicsManager::OT_Murder);
            if (killedByPlayer && player.getClass().getNpcStats(player).isWerewolf())
                player.getClass().getNpcStats(player).addWerewolfKill();
        }

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
            int magnitude = creatureStats.getMagicEffects().get(it->first).getMagnitude();
            if (found != (magnitude > 0))
            {
                std::string itemGmst = it->second;
                std::string item = MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>().find(
                            itemGmst)->getString();
                if (it->first == ESM::MagicEffect::BoundGloves)
                {
                    item = MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>().find(
                                "sMagicBoundLeftGauntletID")->getString();
                    adjustBoundItem(item, magnitude > 0, ptr);
                    item = MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>().find(
                                "sMagicBoundRightGauntletID")->getString();
                    adjustBoundItem(item, magnitude > 0, ptr);
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

        std::map<int, int>& creatureMap = creatureStats.getSummonedCreatureMap();
        for (std::map<int, std::string>::iterator it = summonMap.begin(); it != summonMap.end(); ++it)
        {
            bool found = creatureMap.find(it->first) != creatureMap.end();
            int magnitude = creatureStats.getMagicEffects().get(it->first).getMagnitude();
            if (found != (magnitude > 0))
            {
                if (magnitude > 0)
                {
                    ESM::Position ipos = ptr.getRefData().getPosition();
                    Ogre::Vector3 pos(ipos.pos);
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
                        ref.getPtr().getCellRef().setPosition(ipos);

                        MWMechanics::CreatureStats& summonedCreatureStats = ref.getPtr().getClass().getCreatureStats(ref.getPtr());

                        // Make the summoned creature follow its master and help in fights
                        AiFollow package(ptr.getCellRef().getRefId());
                        summonedCreatureStats.getAiSequence().stack(package, ref.getPtr());
                        int creatureActorId = summonedCreatureStats.getActorId();

                        MWWorld::Ptr placed = MWBase::Environment::get().getWorld()->safePlaceObject(ref.getPtr(),store,ipos);

                        MWRender::Animation* anim = MWBase::Environment::get().getWorld()->getAnimation(placed);
                        if (anim)
                        {
                            const ESM::Static* fx = MWBase::Environment::get().getWorld()->getStore().get<ESM::Static>()
                                    .search("VFX_Summon_Start");
                            if (fx)
                                anim->addEffect("meshes\\" + fx->mModel, -1, false);
                        }

                        creatureMap.insert(std::make_pair(it->first, creatureActorId));
                    }
                }
                else
                {
                    // Effect has ended
                    std::map<int, int>::iterator foundCreature = creatureMap.find(it->first);
                    cleanupSummonedCreature(creatureStats, foundCreature->second);
                    creatureMap.erase(foundCreature);
                }
            }
        }

        for (std::map<int, int>::iterator it = creatureMap.begin(); it != creatureMap.end(); )
        {
            MWWorld::Ptr ptr = MWBase::Environment::get().getWorld()->searchPtrViaActorId(it->second);
            if (!ptr.isEmpty() && ptr.getClass().getCreatureStats(ptr).isDead())
            {
                // Purge the magic effect so a new creature can be summoned if desired
                creatureStats.getActiveSpells().purgeEffect(it->first);
                if (ptr.getClass().hasInventoryStore(ptr))
                    ptr.getClass().getInventoryStore(ptr).purgeEffect(it->first);

                cleanupSummonedCreature(creatureStats, it->second);
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
                        "", Ogre::Vector3(ptr.getRefData().getPosition().pos));

                MWBase::Environment::get().getWorld()->deleteObject(ptr);
            }
            else
                ++it;
        }
    }

    void Actors::calculateNpcStatModifiers (const MWWorld::Ptr& ptr, float duration)
    {
        NpcStats &npcStats = ptr.getClass().getNpcStats(ptr);
        const MagicEffects &effects = npcStats.getMagicEffects();

        // skills
        for(int i = 0;i < ESM::Skill::Length;++i)
        {
            SkillValue& skill = npcStats.getSkill(i);
            skill.setModifier(effects.get(EffectKey(ESM::MagicEffect::FortifySkill, i)).getMagnitude() -
                             effects.get(EffectKey(ESM::MagicEffect::DrainSkill, i)).getMagnitude() -
                             effects.get(EffectKey(ESM::MagicEffect::AbsorbSkill, i)).getMagnitude());

            skill.damage(effects.get(EffectKey(ESM::MagicEffect::DamageSkill, i)).getMagnitude() * duration * 1.5);
            skill.restore(effects.get(EffectKey(ESM::MagicEffect::RestoreSkill, i)).getMagnitude() * duration * 1.5);
        }
    }

    void Actors::updateDrowning(const MWWorld::Ptr& ptr, float duration)
    {
        MWBase::World *world = MWBase::Environment::get().getWorld();
        NpcStats &stats = ptr.getClass().getNpcStats(ptr);
        if(world->isSubmerged(ptr) &&
           stats.getMagicEffects().get(ESM::MagicEffect::WaterBreathing).getMagnitude() == 0)
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

                // Play a drowning sound
                MWBase::SoundManager *sndmgr = MWBase::Environment::get().getSoundManager();
                if(!sndmgr->getSoundPlaying(ptr, "drown"))
                    sndmgr->playSound3D(ptr, "drown", 1.0f, 1.0f);

                if(ptr == world->getPlayerPtr())
                    MWBase::Environment::get().getWindowManager()->activateHitOverlay(false);
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

        MWWorld::InventoryStore &inventoryStore = ptr.getClass().getInventoryStore(ptr);
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
                    if (!ptr.getClass().getCreatureStats (ptr).getAiSequence().isInCombat())
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

    void Actors::updateCrimePersuit(const MWWorld::Ptr& ptr, float duration)
    {
        MWWorld::Ptr player = MWBase::Environment::get().getWorld()->getPlayerPtr();
        if (ptr != player && ptr.getClass().isNpc())
        {
            // get stats of witness
            CreatureStats& creatureStats = ptr.getClass().getCreatureStats(ptr);
            NpcStats& npcStats = ptr.getClass().getNpcStats(ptr);

            if (player.getClass().getNpcStats(player).isWerewolf())
                return;

            if (ptr.getClass().isClass(ptr, "Guard") && creatureStats.getAiSequence().getTypeId() != AiPackage::TypeIdPursue && !creatureStats.getAiSequence().isInCombat())
            {
                const MWWorld::ESMStore& esmStore = MWBase::Environment::get().getWorld()->getStore();
                int cutoff = esmStore.get<ESM::GameSetting>().find("iCrimeThreshold")->getInt();
                // Force dialogue on sight if bounty is greater than the cutoff
                // In vanilla morrowind, the greeting dialogue is scripted to either arrest the player (< 5000 bounty) or attack (>= 5000 bounty)
                if (   player.getClass().getNpcStats(player).getBounty() >= cutoff
                       // TODO: do not run these two every frame. keep an Aware state for each actor and update it every 0.2 s or so?
                    && MWBase::Environment::get().getWorld()->getLOS(ptr, player)
                    && MWBase::Environment::get().getMechanicsManager()->awarenessCheck(player, ptr))
                {
                    static int iCrimeThresholdMultiplier = esmStore.get<ESM::GameSetting>().find("iCrimeThresholdMultiplier")->getInt();
                    if (player.getClass().getNpcStats(player).getBounty() >= cutoff * iCrimeThresholdMultiplier)
                        MWBase::Environment::get().getMechanicsManager()->startCombat(ptr, player);
                    else
                        creatureStats.getAiSequence().stack(AiPursue(player), ptr);
                    creatureStats.setAlarmed(true);
                    npcStats.setCrimeId(MWBase::Environment::get().getWorld()->getPlayer().getNewCrimeId());
                }
            }

            // if I was a witness to a crime
            if (npcStats.getCrimeId() != -1)
            {
                // if you've paid for your crimes and I havent noticed
                if( npcStats.getCrimeId() <= MWBase::Environment::get().getWorld()->getPlayer().getCrimeId() )
                {
                    // Calm witness down
                    if (ptr.getClass().isClass(ptr, "Guard"))
                        creatureStats.getAiSequence().stopPursuit();
                    creatureStats.getAiSequence().stopCombat();

                    // Reset factors to attack
                    creatureStats.setAttacked(false);
                    creatureStats.setAlarmed(false);

                    // Update witness crime id
                    npcStats.setCrimeId(-1);
                }
            }
        }
    }

    Actors::Actors() {}

    Actors::~Actors()
    {
        clear();
    }

    void Actors::addActor (const MWWorld::Ptr& ptr, bool updateImmediately)
    {
        removeActor(ptr);

        MWRender::Animation *anim = MWBase::Environment::get().getWorld()->getAnimation(ptr);
        mActors.insert(std::make_pair(ptr, new CharacterController(ptr, anim)));
        if (updateImmediately)
            mActors[ptr]->update(0);
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

    void Actors::dropActors (const MWWorld::CellStore *cellStore, const MWWorld::Ptr& ignore)
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
        if(!paused)
        {
            static float timerUpdateAITargets = 0;

            // target lists get updated once every 1.0 sec
            if (timerUpdateAITargets >= 1.0f) timerUpdateAITargets = 0;

            // Reset data from previous frame
            for (PtrControllerMap::iterator iter(mActors.begin()); iter != mActors.end(); ++iter)
            {
                // Reset last hit object, which is only valid for one frame
                // Note, the new hit object for this frame may be set by CharacterController::update -> Animation::runAnimation
                // (below)
                iter->first.getClass().getCreatureStats(iter->first).setLastHitObject(std::string());
            }

            MWWorld::Ptr player = MWBase::Environment::get().getWorld()->getPlayerPtr();

            int hostilesCount = 0; // need to know this to play Battle music

            // AI processing is only done within distance of 7168 units to the player. Note the "AI distance" slider doesn't affect this
            // (it only does some throttling for targets beyond the "AI distance", so doesn't give any guarantees as to whether AI will be enabled or not)
            // This distance could be made configurable later, but the setting must be marked with a big warning:
            // using higher values will make a quest in Bloodmoon harder or impossible to complete (bug #1876)
            const float sqrProcessingDistance = 7168*7168;

             // AI and magic effects update
            for(PtrControllerMap::iterator iter(mActors.begin()); iter != mActors.end(); ++iter)
            {
                if (!iter->first.getClass().getCreatureStats(iter->first).isDead())
                {
                    updateActor(iter->first, duration);
                    if (MWBase::Environment::get().getMechanicsManager()->isAIActive() &&
                            Ogre::Vector3(player.getRefData().getPosition().pos).squaredDistance(Ogre::Vector3(iter->first.getRefData().getPosition().pos))
                                    <= sqrProcessingDistance)
                    {
                        if (timerUpdateAITargets == 0)
                        {
                            if (iter->first != player)
                                adjustCommandedActor(iter->first);

                            for(PtrControllerMap::iterator it(mActors.begin()); it != mActors.end(); ++it)
                            {
                                if (it->first == iter->first || iter->first == player) // player is not AI-controlled
                                    continue;
                                engageCombat(iter->first, it->first, it->first == player);
                            }
                        }

                        if (iter->first.getClass().isNpc() && iter->first != player)
                            updateCrimePersuit(iter->first, duration);

                        if (iter->first != player)
                            iter->first.getClass().getCreatureStats(iter->first).getAiSequence().execute(iter->first,iter->second->getAiState(), duration);

                        CreatureStats &stats = iter->first.getClass().getCreatureStats(iter->first);
                        if(!stats.isDead())
                        {
                            if (stats.getAiSequence().isInCombat()) hostilesCount++;
                        }
                    }

                    if(iter->first.getTypeName() == typeid(ESM::NPC).name())
                        updateNpc(iter->first, duration);
                }
            }

            timerUpdateAITargets += duration;

            // Looping magic VFX update
            // Note: we need to do this before any of the animations are updated.
            // Reaching the text keys may trigger Hit / Spellcast (and as such, particles),
            // so updating VFX immediately after that would just remove the particle effects instantly.
            // There needs to be a magic effect update in between.
            for(PtrControllerMap::iterator iter(mActors.begin()); iter != mActors.end(); ++iter)
                iter->second->updateContinuousVfx();

            // Animation/movement update
            CharacterController* playerCharacter = NULL;
            for(PtrControllerMap::iterator iter(mActors.begin()); iter != mActors.end(); ++iter)
            {
                if (iter->first != player &&
                        Ogre::Vector3(player.getRefData().getPosition().pos).squaredDistance(Ogre::Vector3(iter->first.getRefData().getPosition().pos))
                        > sqrProcessingDistance)
                    continue;

                if (iter->first.getClass().getCreatureStats(iter->first).getMagicEffects().get(
                            ESM::MagicEffect::Paralyze).getMagnitude() > 0)
                    iter->second->skipAnim();

                // Handle player last, in case a cell transition occurs by casting a teleportation spell
                // (would invalidate the iterator)
                if (iter->first.getCellRef().getRefId() == "player")
                {
                    playerCharacter = iter->second;
                    continue;
                }
                iter->second->update(duration);
            }

            if (playerCharacter)
                playerCharacter->update(duration);

            for(PtrControllerMap::iterator iter(mActors.begin()); iter != mActors.end(); ++iter)
            {
                const MWWorld::Class &cls = iter->first.getClass();
                CreatureStats &stats = cls.getCreatureStats(iter->first);

                //KnockedOutOneFrameLogic
                //Used for "OnKnockedOut" command
                //Put here to ensure that it's run for PRECISELY one frame.
                if (stats.getKnockedDown() && !stats.getKnockedDownOneFrame() && !stats.getKnockedDownOverOneFrame())
                { //Start it for one frame if nessesary
                    stats.setKnockedDownOneFrame(true);
                }
                else if (stats.getKnockedDownOneFrame() && !stats.getKnockedDownOverOneFrame())
                { //Turn off KnockedOutOneframe
                    stats.setKnockedDownOneFrame(false);
                    stats.setKnockedDownOverOneFrame(true);
                }
            }

            killDeadActors();

            // check if we still have any player enemies to switch music
            static bool isBattleMusic = false;

            if (isBattleMusic && hostilesCount == 0 && !(player.getClass().getCreatureStats(player).isDead() &&
            MWBase::Environment::get().getSoundManager()->isMusicPlaying()))
            {
                MWBase::Environment::get().getSoundManager()->playPlaylist(std::string("Explore"));
                isBattleMusic = false;
            }
            else if (!isBattleMusic && hostilesCount > 0)
            {
                MWBase::Environment::get().getSoundManager()->playPlaylist(std::string("Battle"));
                isBattleMusic = true;
            }

            static float sneakTimer = 0.f; // times update of sneak icon

            // if player is in sneak state see if anyone detects him
            if (player.getClass().getCreatureStats(player).getMovementFlag(MWMechanics::CreatureStats::Flag_Sneak))
            {
                static float sneakSkillTimer = 0.f; // times sneak skill progress from "avoid notice"

                const MWWorld::ESMStore& esmStore = MWBase::Environment::get().getWorld()->getStore();
                const int radius = esmStore.get<ESM::GameSetting>().find("fSneakUseDist")->getInt();

                static float fSneakUseDelay = esmStore.get<ESM::GameSetting>().find("fSneakUseDelay")->getFloat();

                if (sneakTimer >= fSneakUseDelay)
                    sneakTimer = 0.f;

                if (sneakTimer == 0.f)
                {
                    // Set when an NPC is within line of sight and distance, but is still unaware. Used for skill progress.
                    bool avoidedNotice = false;

                    bool detected = false;

                    for (PtrControllerMap::iterator iter(mActors.begin()); iter != mActors.end(); ++iter)
                    {
                        if (iter->first == player)  // not the player
                            continue;

                        // is the player in range and can they be detected
                        if (Ogre::Vector3(iter->first.getRefData().getPosition().pos).squaredDistance(Ogre::Vector3(player.getRefData().getPosition().pos)) <= radius*radius
                            && MWBase::Environment::get().getWorld()->getLOS(player, iter->first))
                        {
                            if (MWBase::Environment::get().getMechanicsManager()->awarenessCheck(player, iter->first))
                            {
                                detected = true;
                                avoidedNotice = false;
                                MWBase::Environment::get().getWindowManager()->setSneakVisibility(false);
                                break;
                            }
                            else if (!detected)
                                avoidedNotice = true;
                        }
                    }

                    if (sneakSkillTimer >= fSneakUseDelay)
                        sneakSkillTimer = 0.f;

                    if (avoidedNotice && sneakSkillTimer == 0.f)
                        player.getClass().skillUsageSucceeded(player, ESM::Skill::Sneak, 0);

                    if (!detected)
                        MWBase::Environment::get().getWindowManager()->setSneakVisibility(true);
                }
                sneakTimer += duration;
                sneakSkillTimer += duration;
            }
            else
            {
                sneakTimer = 0.f;
                MWBase::Environment::get().getWindowManager()->setSneakVisibility(false);
            }
        }
    }

    void Actors::killDeadActors()
    {
        for(PtrControllerMap::iterator iter(mActors.begin()); iter != mActors.end(); ++iter)
        {
            const MWWorld::Class &cls = iter->first.getClass();
            CreatureStats &stats = cls.getCreatureStats(iter->first);

            if(!stats.isDead())
            {
                if(iter->second->isDead())
                {
                    // Actor has been resurrected. Notify the CharacterController and re-enable collision.
                    MWBase::Environment::get().getWorld()->enableActorCollision(iter->first, true);
                    iter->second->resurrect();
                }

                if(!stats.isDead())
                    continue;
            }

            if (iter->second->kill())
            {
                iter->first.getClass().getCreatureStats(iter->first).notifyDied();

                ++mDeathCount[Misc::StringUtils::lowerCase(iter->first.getCellRef().getRefId())];

                // Make sure spell effects with CasterLinked flag are removed
                for (PtrControllerMap::iterator iter2(mActors.begin());iter2 != mActors.end();++iter2)
                {
                    MWMechanics::ActiveSpells& spells = iter2->first.getClass().getCreatureStats(iter2->first).getActiveSpells();
                    spells.purge(stats.getActorId());
                }

                // Apply soultrap
                if (iter->first.getTypeName() == typeid(ESM::Creature).name())
                {
                    SoulTrap soulTrap (iter->first);
                    stats.getActiveSpells().visitEffectSources(soulTrap);
                }

                // Reset magic effects and recalculate derived effects
                // One case where we need this is to make sure bound items are removed upon death
                stats.modifyMagicEffects(MWMechanics::MagicEffects());
                stats.getActiveSpells().clear();
                calculateCreatureStatModifiers(iter->first, 0);

                MWBase::Environment::get().getWorld()->enableActorCollision(iter->first, false);

                if (cls.isEssential(iter->first))
                    MWBase::Environment::get().getWindowManager()->messageBox("#{sKilledEssential}");
            }
        }
    }

    void Actors::restoreDynamicStats(bool sleep)
    {
        for(PtrControllerMap::iterator iter(mActors.begin());iter != mActors.end();++iter)
            restoreDynamicStats(iter->first, sleep);
    }

    int Actors::getHoursToRest(const MWWorld::Ptr &ptr) const
    {
        float healthPerHour, magickaPerHour;
        getRestorationPerHourOfSleep(ptr, healthPerHour, magickaPerHour);

        CreatureStats& stats = ptr.getClass().getCreatureStats(ptr);

        float healthHours  = healthPerHour > 0
                             ? (stats.getHealth().getModified() - stats.getHealth().getCurrent()) / healthPerHour
                             : 1.0f;
        float magickaHours = magickaPerHour > 0
                              ? (stats.getMagicka().getModified() - stats.getMagicka().getCurrent()) / magickaPerHour
                              : 1.0f;

        int autoHours = std::ceil(std::max(1.f, std::max(healthHours, magickaHours)));
        return autoHours;
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

    void Actors::getObjectsInRange(const Ogre::Vector3& position, float radius, std::vector<MWWorld::Ptr>& out)
    {
        for (PtrControllerMap::iterator iter = mActors.begin(); iter != mActors.end(); ++iter)
        {
            if (Ogre::Vector3(iter->first.getRefData().getPosition().pos).squaredDistance(position) <= radius*radius)
                out.push_back(iter->first);
        }
    }

    std::list<MWWorld::Ptr> Actors::getActorsFollowing(const MWWorld::Ptr& actor)
    {
        std::list<MWWorld::Ptr> list;
        for(PtrControllerMap::iterator iter(mActors.begin());iter != mActors.end();++iter)
        {
            const MWWorld::Class &cls = iter->first.getClass();
            CreatureStats &stats = cls.getCreatureStats(iter->first);
            if (stats.isDead())
                continue;

            // An actor counts as following if AiFollow is the current AiPackage, or there are only Combat packages before the AiFollow package
            for (std::list<MWMechanics::AiPackage*>::const_iterator it = stats.getAiSequence().begin(); it != stats.getAiSequence().end(); ++it)
            {
                if ((*it)->getTypeId() == MWMechanics::AiPackage::TypeIdFollow)
                {
                    MWWorld::Ptr followTarget = dynamic_cast<MWMechanics::AiFollow*>(*it)->getTarget();
                    if (followTarget.isEmpty())
                        continue;
                    if (followTarget == actor)
                        list.push_back(iter->first);
                    else
                        break;
                }
                else if ((*it)->getTypeId() != MWMechanics::AiPackage::TypeIdCombat)
                    break;
            }
        }
        return list;
    }

    std::list<MWWorld::Ptr> Actors::getActorsFighting(const MWWorld::Ptr& actor) {
        std::list<MWWorld::Ptr> list;
        std::vector<MWWorld::Ptr> neighbors;
        Ogre::Vector3 position = Ogre::Vector3(actor.getRefData().getPosition().pos);
        getObjectsInRange(position,
            MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>().find("fAlarmRadius")->getFloat(),
            neighbors); //only care about those within the alarm disance
        for(std::vector<MWWorld::Ptr>::iterator iter(neighbors.begin());iter != neighbors.end();++iter)
        {
            const MWWorld::Class &cls = iter->getClass();
            CreatureStats &stats = cls.getCreatureStats(*iter);
            if (!stats.isDead() && stats.getAiSequence().isInCombat(actor))
                list.push_front(*iter);
        }
        return list;
    }

    void Actors::write (ESM::ESMWriter& writer, Loading::Listener& listener) const
    {
        writer.startRecord(ESM::REC_DCOU);
        for (std::map<std::string, int>::const_iterator it = mDeathCount.begin(); it != mDeathCount.end(); ++it)
        {
            writer.writeHNString("ID__", it->first);
            writer.writeHNT ("COUN", it->second);
        }
        writer.endRecord(ESM::REC_DCOU);

        listener.increaseProgress(1);
    }

    void Actors::readRecord (ESM::ESMReader& reader, int32_t type)
    {
        if (type == ESM::REC_DCOU)
        {
            while (reader.isNextSub("ID__"))
            {
                std::string id = reader.getHString();
                int count;
                reader.getHNT (count, "COUN");
                mDeathCount[id] = count;
            }
        }
    }

    void Actors::clear()
    {
        PtrControllerMap::iterator it(mActors.begin());
        for (; it != mActors.end(); ++it)
        {
            delete it->second;
            it->second = NULL;
        }
        mActors.clear();
        mDeathCount.clear();
    }

    void Actors::updateMagicEffects(const MWWorld::Ptr &ptr)
    {
        adjustMagicEffects(ptr);
        calculateCreatureStatModifiers(ptr, 0.f);
        if (ptr.getClass().isNpc())
            calculateNpcStatModifiers(ptr, 0.f);
    }
}

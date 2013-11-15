
#include "actors.hpp"

#include <typeinfo>

#include <OgreVector3.h>

#include <components/esm/loadnpc.hpp>

#include "../mwworld/esmstore.hpp"

#include "../mwworld/class.hpp"
#include "../mwworld/inventorystore.hpp"
#include "../mwworld/player.hpp"

#include "../mwbase/world.hpp"
#include "../mwbase/environment.hpp"
#include "../mwbase/windowmanager.hpp"
#include "../mwbase/soundmanager.hpp"

#include "npcstats.hpp"
#include "creaturestats.hpp"
#include "movement.hpp"

namespace MWMechanics
{
    void Actors::updateActor (const MWWorld::Ptr& ptr, float duration)
    {
        // magic effects
        adjustMagicEffects (ptr);
        calculateDynamicStats (ptr);
        calculateCreatureStatModifiers (ptr);

        if(!MWBase::Environment::get().getWindowManager()->isGuiMode())
        {
            // AI
            CreatureStats& creatureStats =  MWWorld::Class::get (ptr).getCreatureStats (ptr);
            creatureStats.getAiSequence().execute (ptr);

            // fatigue restoration
            calculateRestoration(ptr, duration);
        }
    }

    void Actors::updateNpc (const MWWorld::Ptr& ptr, float duration, bool paused)
    {
        if(!paused)
        {
            updateDrowning(ptr, duration);
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

        MagicEffects diff = MagicEffects::diff (creatureStats.getMagicEffects(), now);

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

            bool stunted = stats.getMagicEffects ().get(MWMechanics::EffectKey(ESM::MagicEffect::StuntedMagicka)).mMagnitude > 0;

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

    void Actors::calculateCreatureStatModifiers (const MWWorld::Ptr& ptr)
    {
        CreatureStats &creatureStats = MWWorld::Class::get(ptr).getCreatureStats(ptr);
        const MagicEffects &effects = creatureStats.getMagicEffects();

        // attributes
        for(int i = 0;i < ESM::Attribute::Length;++i)
        {
            Stat<int> stat = creatureStats.getAttribute(i);
            stat.setModifier(effects.get(EffectKey(ESM::MagicEffect::FortifyAttribute, i)).mMagnitude -
                             effects.get(EffectKey(ESM::MagicEffect::DrainAttribute, i)).mMagnitude);

            creatureStats.setAttribute(i, stat);
        }

        // dynamic stats
        for(int i = 0;i < 3;++i)
        {
            DynamicStat<float> stat = creatureStats.getDynamic(i);
            stat.setModifier(effects.get(EffectKey(80+i)).mMagnitude -
                             effects.get(EffectKey(18+i)).mMagnitude);

            creatureStats.setDynamic(i, stat);
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
                ptr.getClass().setActorHealth(ptr, stats.getHealth().getCurrent() - 3.0f*duration);

                // Play a drowning sound as necessary for the player
                if(ptr == world->getPlayer().getPlayer())
                {
                    MWBase::SoundManager *sndmgr = MWBase::Environment::get().getSoundManager();
                    if(!sndmgr->getSoundPlaying(MWWorld::Ptr(), "drown"))
                        sndmgr->playSound("drown", 1.0f, 1.0f);
                }
            }
        }
        else
            stats.setTimeToStartDrowning(20);
    }

    void Actors::updateEquippedLight (const MWWorld::Ptr& ptr, float duration)
    {
        //If holding a light...
        MWWorld::InventoryStore &inventoryStore = MWWorld::Class::get(ptr).getInventoryStore(ptr);
        MWWorld::ContainerStoreIterator heldIter =
                inventoryStore.getSlot(MWWorld::InventoryStore::Slot_CarriedLeft);

        if(heldIter.getType() == MWWorld::ContainerStore::Type_Light)
        {
            // Use time from the player's light
            bool isPlayer = ptr.getRefData().getHandle()=="player";
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
                        heldIter->getRefData().setCount(0); // remove it
                        return;
                    }
                }
            }

            // Both NPC and player lights extinguish in water.
            if(MWBase::Environment::get().getWorld()->isSwimming(ptr))
            {
                heldIter->getRefData().setCount(0); // remove it

                // ...But, only the player makes a sound.
                if(isPlayer)
                    MWBase::Environment::get().getSoundManager()->playSound("torch out",
                            1.0, 1.0, MWBase::SoundManager::Play_TypeSfx, MWBase::SoundManager::Play_NoEnv);
            }
        }
    }

    Actors::Actors() : mDuration (0) {}

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

    void Actors::dropActors (const MWWorld::Ptr::CellStore *cellStore)
    {
        PtrControllerMap::iterator iter = mActors.begin();
        while(iter != mActors.end())
        {
            if(iter->first.getCell()==cellStore)
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
        mDuration += duration;

        //if (mDuration>=0.25)
        {
            float totalDuration = mDuration;
            mDuration = 0;

            for(PtrControllerMap::iterator iter(mActors.begin());iter != mActors.end();iter++)
            {
                const MWWorld::Class &cls = MWWorld::Class::get(iter->first);
                CreatureStats &stats = cls.getCreatureStats(iter->first);

                stats.setLastHitObject(std::string());
                if(!stats.isDead())
                {
                    if(iter->second->isDead())
                        iter->second->resurrect();

                    updateActor(iter->first, totalDuration);
                    if(iter->first.getTypeName() == typeid(ESM::NPC).name())
                        updateNpc(iter->first, totalDuration, paused);

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

                ++mDeathCount[cls.getId(iter->first)];

                if(cls.isEssential(iter->first))
                    MWBase::Environment::get().getWindowManager()->messageBox("#{sKilledEssential}");
            }
        }

        if(!paused)
        {
            for(PtrControllerMap::iterator iter(mActors.begin());iter != mActors.end();++iter)
                iter->second->update(duration);
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

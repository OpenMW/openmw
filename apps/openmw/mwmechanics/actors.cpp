
#include "actors.hpp"

#include <typeinfo>

#include <OgreVector3.h>

#include <components/esm/loadnpc.hpp>

#include "../mwworld/esmstore.hpp"

#include "../mwworld/class.hpp"
#include "../mwworld/inventorystore.hpp"

#include "../mwbase/world.hpp"
#include "../mwbase/environment.hpp"
#include "../mwbase/windowmanager.hpp"

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

        // AI
        if(!MWBase::Environment::get().getWindowManager()->isGuiMode())
        {
            CreatureStats& creatureStats =  MWWorld::Class::get (ptr).getCreatureStats (ptr);
            creatureStats.getAiSequence().execute (ptr);
        }
    }

    void Actors::updateNpc (const MWWorld::Ptr& ptr, float duration, bool paused)
    {
        if(!paused)
            updateDrowning(ptr, duration);
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

        int strength        = creatureStats.getAttribute(0).getBase();
        int intelligence    = creatureStats.getAttribute(1).getBase();
        int willpower       = creatureStats.getAttribute(2).getBase();
        int agility         = creatureStats.getAttribute(3).getBase();
        int endurance       = creatureStats.getAttribute(5).getBase();

        double magickaFactor =
            creatureStats.getMagicEffects().get (EffectKey (ESM::MagicEffect::FortifyMaximumMagicka)).mMagnitude * 0.1 + 0.5;

        DynamicStat<float> magicka = creatureStats.getMagicka();
        magicka.setBase (static_cast<int> (intelligence + magickaFactor * intelligence));
        creatureStats.setMagicka (magicka);

        DynamicStat<float> fatigue = creatureStats.getFatigue();
        fatigue.setBase (strength+willpower+agility+endurance);
        creatureStats.setFatigue (fatigue);
    }

    void Actors::calculateRestoration (const MWWorld::Ptr& ptr, float duration)
    {
        CreatureStats& stats = MWWorld::Class::get (ptr).getCreatureStats (ptr);

        if (duration == 3600)
        {
            bool stunted = stats.getMagicEffects ().get(MWMechanics::EffectKey(ESM::MagicEffect::StuntedMagicka)).mMagnitude > 0;

            int endurance = stats.getAttribute (ESM::Attribute::Endurance).getModified ();

            DynamicStat<float> health = stats.getHealth();
            health.setCurrent (health.getCurrent() + 0.1 * endurance);
            stats.setHealth (health);

            const MWWorld::ESMStore& store = MWBase::Environment::get().getWorld()->getStore();

            float fFatigueReturnBase = store.get<ESM::GameSetting>().find("fFatigueReturnBase")->getFloat ();
            float fFatigueReturnMult = store.get<ESM::GameSetting>().find("fFatigueReturnMult")->getFloat ();
            float fEndFatigueMult = store.get<ESM::GameSetting>().find("fEndFatigueMult")->getFloat ();

            float capacity = MWWorld::Class::get(ptr).getCapacity(ptr);
            float encumbrance = MWWorld::Class::get(ptr).getEncumbrance(ptr);
            float normalizedEncumbrance = (capacity == 0 ? 1 : encumbrance/capacity);
            if (normalizedEncumbrance > 1)
                normalizedEncumbrance = 1;

            float x = fFatigueReturnBase + fFatigueReturnMult * (1 - normalizedEncumbrance);
            x *= fEndFatigueMult * endurance;

            DynamicStat<float> fatigue = stats.getFatigue();
            fatigue.setCurrent (fatigue.getCurrent() + 3600 * x);
            stats.setFatigue (fatigue);

            if (!stunted)
            {
                float fRestMagicMult = store.get<ESM::GameSetting>().find("fRestMagicMult")->getFloat ();

                DynamicStat<float> magicka = stats.getMagicka();
                magicka.setCurrent (magicka.getCurrent()
                    + fRestMagicMult * stats.getAttribute(ESM::Attribute::Intelligence).getModified());
                stats.setMagicka (magicka);
            }
        }
    }

    void Actors::calculateCreatureStatModifiers (const MWWorld::Ptr& ptr)
    {
        CreatureStats& creatureStats = MWWorld::Class::get (ptr).getCreatureStats (ptr);

        // attributes
        for (int i=0; i<8; ++i)
        {
            int modifier =
                creatureStats.getMagicEffects().get (EffectKey (ESM::MagicEffect::FortifyAttribute, i)).mMagnitude;

            modifier -= creatureStats.getMagicEffects().get (EffectKey (ESM::MagicEffect::DrainAttribute, i)).mMagnitude;

            creatureStats.getAttribute(i).setModifier (modifier);
        }

        // dynamic stats
        MagicEffects effects = creatureStats.getMagicEffects();

        for (int i=0; i<3; ++i)
        {
            DynamicStat<float> stat = creatureStats.getDynamic (i);

            stat.setModifier (
                effects.get (EffectKey(80+i)).mMagnitude - effects.get (EffectKey(18+i)).mMagnitude);

            creatureStats.setDynamic (i, stat);
        }
    }

    void Actors::updateDrowning(const MWWorld::Ptr& ptr, float duration)
    {
        NpcStats &stats = MWWorld::Class::get(ptr).getNpcStats(ptr);
        if(MWBase::Environment::get().getWorld()->isSubmerged(ptr) && 
           stats.getMagicEffects().get(ESM::MagicEffect::WaterBreathing).mMagnitude == 0)
        {
            if(stats.getFatigue().getCurrent() == 0)
                stats.setTimeToStartDrowning(0);

            float timeLeft = stats.getTimeToStartDrowning()-duration;
            if(timeLeft < 0.0f)
                timeLeft = 0.0f;

            stats.setTimeToStartDrowning(timeLeft);
            if(timeLeft == 0.0f)
                stats.setLastDrowningHitTime(stats.getLastDrowningHitTime()+duration);
        }
        else
        {
            stats.setTimeToStartDrowning(20);
            stats.setLastDrowningHitTime(0);
        }
        //if npc is drowning and it's time to hit, then hit
        while(stats.getTimeToStartDrowning() == 0.0f && stats.getLastDrowningHitTime() > 0.33f)
        {
            stats.setLastDrowningHitTime(stats.getLastDrowningHitTime()-0.33f);
            //fixme: replace it with something different once screen hit effects are implemented (blood on screen)
            MWWorld::Class::get(ptr).setActorHealth(ptr, stats.getHealth().getCurrent()-1.0f);
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

                // workaround: always keep player alive for now
                // \todo remove workaround, once player death can be handled
                if(iter->first.getRefData().getHandle()=="player")
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

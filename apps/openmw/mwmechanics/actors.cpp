
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

#include "creaturestats.hpp"

namespace MWMechanics
{
    void Actors::updateActor (const MWWorld::Ptr& ptr, float duration)
    {
        // magic effects
        adjustMagicEffects (ptr);
        calculateDynamicStats (ptr);
        calculateCreatureStatModifiers (ptr);

        // AI
        CreatureStats& creatureStats =  MWWorld::Class::get (ptr).getCreatureStats (ptr);
        creatureStats.getAiSequence().execute (ptr);
    }

    void Actors::updateNpc (const MWWorld::Ptr& ptr, float duration, bool paused)
    {
        if (!paused && ptr.getRefData().getHandle()!="player")
            MWWorld::Class::get (ptr).getInventoryStore (ptr).autoEquip (
                MWWorld::Class::get (ptr).getNpcStats (ptr));
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

        DynamicStat<float> health = creatureStats.getHealth();
        health.setBase (static_cast<int> (0.5 * (strength + endurance)) + creatureStats.getLevelHealthBonus ());
        creatureStats.setHealth (health);

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

    Actors::Actors() : mDuration (0) {}

    void Actors::addActor (const MWWorld::Ptr& ptr)
    {
        // erase previous death events since we are currently only tracking them while in an active cell
        MWWorld::Class::get (ptr).getCreatureStats (ptr).clearHasDied();

        MWRender::Animation *anim = MWBase::Environment::get().getWorld()->getAnimation(ptr);
        if(!MWWorld::Class::get(ptr).getCreatureStats(ptr).isDead())
            mActors.insert(std::make_pair(ptr, CharacterController(ptr, anim, CharState_Idle, true)));
        else
            mActors.insert(std::make_pair(ptr, CharacterController(ptr, anim, CharState_Death1, false)));
    }

    void Actors::removeActor (const MWWorld::Ptr& ptr)
    {
        PtrControllerMap::iterator iter = mActors.find(ptr);
        if(iter != mActors.end())
            mActors.erase(iter);
    }

    void Actors::updateActor(const MWWorld::Ptr &old, const MWWorld::Ptr &ptr)
    {
        PtrControllerMap::iterator iter = mActors.find(old);
        if(iter != mActors.end())
        {
            CharacterController ctrl = iter->second;
            mActors.erase(iter);

            ctrl.updatePtr(ptr);
            mActors.insert(std::make_pair(ptr, ctrl));
        }
    }

    void Actors::dropActors (const MWWorld::Ptr::CellStore *cellStore)
    {
        PtrControllerMap::iterator iter = mActors.begin();
        while(iter != mActors.end())
        {
            if(iter->first.getCell()==cellStore)
                mActors.erase(iter++);
            else
                ++iter;
        }
    }

    void Actors::update (float duration, bool paused)
    {
        mDuration += duration;

        if (mDuration>=0.25)
        {
            float totalDuration = mDuration;
            mDuration = 0;

            for(PtrControllerMap::iterator iter(mActors.begin());iter != mActors.end();iter++)
            {
                if(!MWWorld::Class::get(iter->first).getCreatureStats(iter->first).isDead())
                {
                    if(iter->second.getState() >= CharState_Death1)
                        iter->second.setState(CharState_Idle, true);

                    updateActor(iter->first, totalDuration);
                    if(iter->first.getTypeName() == typeid(ESM::NPC).name())
                        updateNpc(iter->first, totalDuration, paused);

                    if(!MWWorld::Class::get(iter->first).getCreatureStats(iter->first).isDead())
                        continue;
                }

                // workaround: always keep player alive for now
                // \todo remove workaround, once player death can be handled
                if(iter->first.getRefData().getHandle()=="player")
                {
                    MWMechanics::DynamicStat<float> stat (
                        MWWorld::Class::get(iter->first).getCreatureStats(iter->first).getHealth());

                    if (stat.getModified()<1)
                    {
                        stat.setModified (1, 0);
                        MWWorld::Class::get(iter->first).getCreatureStats(iter->first).setHealth(stat);
                    }

                    MWWorld::Class::get(iter->first).getCreatureStats(iter->first).resurrect();
                    continue;
                }

                if(iter->second.getState() >= CharState_Death1)
                    continue;

                iter->second.setState(CharState_Death1, false);

                ++mDeathCount[MWWorld::Class::get(iter->first).getId(iter->first)];

                if(MWWorld::Class::get(iter->first).isEssential(iter->first))
                    MWBase::Environment::get().getWindowManager()->messageBox(
                        "#{sKilledEssential}", std::vector<std::string>());
            }
        }

        if(!paused)
        {
            mMovement.reserve(mActors.size());

            for(PtrControllerMap::iterator iter(mActors.begin());iter != mActors.end();++iter)
            {
                Ogre::Vector3 movement = iter->second.update(duration);
                mMovement.push_back(std::make_pair(iter->first, movement));
            }
            MWBase::Environment::get().getWorld()->doPhysics(mMovement, duration);

            mMovement.clear();
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

    void Actors::playAnimationGroup(const MWWorld::Ptr& ptr, const std::string& groupName, int mode, int number)
    {
        PtrControllerMap::iterator iter = mActors.find(ptr);
        if(iter != mActors.end())
            iter->second.playGroup(groupName, mode, number);
    }
    void Actors::skipAnimation(const MWWorld::Ptr& ptr)
    {
        PtrControllerMap::iterator iter = mActors.find(ptr);
        if(iter != mActors.end())
            iter->second.skipAnim();
    }
}

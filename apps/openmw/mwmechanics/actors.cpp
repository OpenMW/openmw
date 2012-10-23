
#include "actors.hpp"

#include <typeinfo>

#include <OgreVector3.h>

#include <components/esm/loadnpc.hpp>

#include <components/esm_store/store.hpp>

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
            creatureStats.getMagicEffects().get (EffectKey (84)).mMagnitude * 0.1 + 0.5;

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
            // stunted magicka
            bool stunted = stats.getMagicEffects ().get(MWMechanics::EffectKey(136)).mMagnitude > 0;

            int endurance = stats.getAttribute (ESM::Attribute::Endurance).getModified ();
            
            DynamicStat<float> health = stats.getHealth();
            health.setCurrent (health.getCurrent() + 0.1 * endurance);
            stats.setHealth (health);

            const ESMS::ESMStore& store = MWBase::Environment::get().getWorld()->getStore();

            float fFatigueReturnBase = store.gameSettings.find("fFatigueReturnBase")->getFloat ();
            float fFatigueReturnMult = store.gameSettings.find("fFatigueReturnMult")->getFloat ();
            float fEndFatigueMult = store.gameSettings.find("fEndFatigueMult")->getFloat ();

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
                float fRestMagicMult = store.gameSettings.find("fRestMagicMult")->getFloat ();
                
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
                creatureStats.getMagicEffects().get (EffectKey (79, i)).mMagnitude;

            modifier -= creatureStats.getMagicEffects().get (EffectKey (17, i)).mMagnitude;

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
        if (!MWWorld::Class::get (ptr).getCreatureStats (ptr).isDead())
            mActors.insert (ptr);
    }

    void Actors::removeActor (const MWWorld::Ptr& ptr)
    {
        std::set<MWWorld::Ptr>::iterator iter = mActors.find (ptr);

        if (iter!=mActors.end())
            mActors.erase (iter);
    }

    void Actors::dropActors (const MWWorld::Ptr::CellStore *cellStore)
    {
        std::set<MWWorld::Ptr>::iterator iter = mActors.begin();

        while (iter!=mActors.end())
            if (iter->getCell()==cellStore)
            {
                mActors.erase (iter++);
            }
            else
                ++iter;
    }

    void Actors::update (std::vector<std::pair<std::string, Ogre::Vector3> >& movement, float duration,
        bool paused)
    {
        mDuration += duration;

        if (mDuration>=0.25)
        {
            float totalDuration = mDuration;
            mDuration = 0;
            
            std::set<MWWorld::Ptr>::iterator iter (mActors.begin());

            while (iter!=mActors.end())
            {
                if (!MWWorld::Class::get (*iter).getCreatureStats (*iter).isDead())
                {
                    updateActor (*iter, totalDuration);

                    if (iter->getTypeName()==typeid (ESM::NPC).name())
                        updateNpc (*iter, totalDuration, paused);
                }

                if (MWWorld::Class::get (*iter).getCreatureStats (*iter).isDead())
                    mActors.erase (iter++);
                else
                    ++iter;
            }
        }

        for (std::set<MWWorld::Ptr>::iterator iter (mActors.begin()); iter!=mActors.end();
            ++iter)
        {
            Ogre::Vector3 vector = MWWorld::Class::get (*iter).getMovementVector (*iter);

            if (vector!=Ogre::Vector3::ZERO)
                movement.push_back (std::make_pair (iter->getRefData().getHandle(), vector));
        }
    }

    void Actors::restoreDynamicStats()
    {
        for (std::set<MWWorld::Ptr>::iterator iter (mActors.begin()); iter!=mActors.end(); ++iter)
        {
            calculateRestoration (*iter, 3600);
        }
    }
}

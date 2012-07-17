
#include "actors.hpp"

#include <typeinfo>

#include <OgreVector3.h>

#include <components/esm/loadnpc.hpp>

#include "../mwworld/class.hpp"
#include "../mwworld/inventorystore.hpp"

#include "creaturestats.hpp"

namespace MWMechanics
{
    void Actors::updateActor (const MWWorld::Ptr& ptr, float duration)
    {
        // magic effects
        adjustMagicEffects (ptr);
        calculateCreatureStatModifiers (ptr);
        calculateDynamicStats (ptr);
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

        MagicEffects now = creatureStats.mSpells.getMagicEffects();

        if (creature.getTypeName()==typeid (ESM::NPC).name())
        {
            MWWorld::InventoryStore& store = MWWorld::Class::get (creature).getInventoryStore (creature);
            now += store.getMagicEffects();
        }

        now += creatureStats.mActiveSpells.getMagicEffects();

        MagicEffects diff = MagicEffects::diff (creatureStats.mMagicEffects, now);

        creatureStats.mMagicEffects = now;

        // TODO apply diff to other stats
    }

    void Actors::calculateDynamicStats (const MWWorld::Ptr& ptr)
    {
        CreatureStats& creatureStats = MWWorld::Class::get (ptr).getCreatureStats (ptr);

        int strength = creatureStats.mAttributes[0].getBase();
        int intelligence = creatureStats.mAttributes[1].getBase();
        int willpower = creatureStats.mAttributes[2].getBase();
        int agility = creatureStats.mAttributes[3].getBase();
        int endurance = creatureStats.mAttributes[5].getBase();

        double magickaFactor = creatureStats.mMagicEffects.get (EffectKey (84)).mMagnitude*0.1 + 0.5;

        creatureStats.mDynamic[0].setBase (static_cast<int> (0.5 * (strength + endurance)));
        creatureStats.mDynamic[1].setBase (static_cast<int> (intelligence +
            magickaFactor * intelligence));
        creatureStats.mDynamic[2].setBase (strength+willpower+agility+endurance);
    }

    void Actors::calculateCreatureStatModifiers (const MWWorld::Ptr& ptr)
    {
        CreatureStats& creatureStats = MWWorld::Class::get (ptr).getCreatureStats (ptr);

        // attributes
        for (int i=0; i<5; ++i)
        {
            int modifier = creatureStats.mMagicEffects.get (EffectKey (79, i)).mMagnitude
                - creatureStats.mMagicEffects.get (EffectKey (17, i)).mMagnitude;

            creatureStats.mAttributes[i].setModifier (modifier);
        }

        // dynamic stats
        for (int i=0; i<3; ++i)
        {
            int modifier = creatureStats.mMagicEffects.get (EffectKey (80+i)).mMagnitude
                - creatureStats.mMagicEffects.get (EffectKey (18+i)).mMagnitude;

            creatureStats.mDynamic[i].setModifier (modifier);
        }
    }

    Actors::Actors() : mDuration (0) {}

    void Actors::addActor (const MWWorld::Ptr& ptr)
    {
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

            for (std::set<MWWorld::Ptr>::iterator iter (mActors.begin()); iter!=mActors.end(); ++iter)
            {
                updateActor (*iter, totalDuration);

                if (iter->getTypeName()==typeid (ESM::NPC).name())
                    updateNpc (*iter, totalDuration, paused);
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
}


#include "mechanicsmanagerimp.hpp"

#include <components/esm_store/store.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwbase/windowmanager.hpp"

#include "../mwworld/class.hpp"
#include "../mwworld/player.hpp"

namespace MWMechanics
{
    void MechanicsManager::buildPlayer()
    {
        MWWorld::Ptr ptr = MWBase::Environment::get().getWorld()->getPlayer().getPlayer();

        MWMechanics::CreatureStats& creatureStats = MWWorld::Class::get (ptr).getCreatureStats (ptr);
        MWMechanics::NpcStats& npcStats = MWWorld::Class::get (ptr).getNpcStats (ptr);

        const ESM::NPC *player = ptr.get<ESM::NPC>()->base;

        // reset
        creatureStats.setLevel(player->mNpdt52.mLevel);
        creatureStats.getSpells().clear();
        creatureStats.setMagicEffects(MagicEffects());

        for (int i=0; i<27; ++i)
            npcStats.getSkill (i).setBase (player->mNpdt52.mSkills[i]);

        creatureStats.getAttribute(0).setBase (player->mNpdt52.mStrength);
        creatureStats.getAttribute(1).setBase (player->mNpdt52.mIntelligence);
        creatureStats.getAttribute(2).setBase (player->mNpdt52.mWillpower);
        creatureStats.getAttribute(3).setBase (player->mNpdt52.mAgility);
        creatureStats.getAttribute(4).setBase (player->mNpdt52.mSpeed);
        creatureStats.getAttribute(5).setBase (player->mNpdt52.mEndurance);
        creatureStats.getAttribute(6).setBase (player->mNpdt52.mPersonality);
        creatureStats.getAttribute(7).setBase (player->mNpdt52.mLuck);

        // race
        if (mRaceSelected)
        {
            const ESM::Race *race =
                MWBase::Environment::get().getWorld()->getStore().races.find (
                MWBase::Environment::get().getWorld()->getPlayer().getRace());

            bool male = MWBase::Environment::get().getWorld()->getPlayer().isMale();

            for (int i=0; i<8; ++i)
            {
                const ESM::Race::MaleFemale *attribute = 0;
                switch (i)
                {
                    case 0: attribute = &race->mData.mStrength; break;
                    case 1: attribute = &race->mData.mIntelligence; break;
                    case 2: attribute = &race->mData.mWillpower; break;
                    case 3: attribute = &race->mData.mAgility; break;
                    case 4: attribute = &race->mData.mSpeed; break;
                    case 5: attribute = &race->mData.mEndurance; break;
                    case 6: attribute = &race->mData.mPersonality; break;
                    case 7: attribute = &race->mData.mLuck; break;
                }

                creatureStats.getAttribute(i).setBase (
                    static_cast<int> (male ? attribute->mMale : attribute->mFemale));
            }

            for (int i=0; i<27; ++i)
            {
                int bonus = 0;
                
                for (int i2=0; i2<7; ++i2)
                    if (race->mData.mBonus[i2].mSkill==i)
                    {
                        bonus = race->mData.mBonus[i2].mBonus;
                        break;
                    }
            
                npcStats.getSkill (i).setBase (5 + bonus);
            }

            for (std::vector<std::string>::const_iterator iter (race->mPowers.mList.begin());
                iter!=race->mPowers.mList.end(); ++iter)
            {
                creatureStats.getSpells().add (*iter);
            }
        }

        // birthsign
        if (!MWBase::Environment::get().getWorld()->getPlayer().getBirthsign().empty())
        {
            const ESM::BirthSign *sign =
                MWBase::Environment::get().getWorld()->getStore().birthSigns.find (
                MWBase::Environment::get().getWorld()->getPlayer().getBirthsign());

            for (std::vector<std::string>::const_iterator iter (sign->mPowers.mList.begin());
                iter!=sign->mPowers.mList.end(); ++iter)
            {
                creatureStats.getSpells().add (*iter);
            }
        }

        // class
        if (mClassSelected)
        {
            const ESM::Class& class_ = MWBase::Environment::get().getWorld()->getPlayer().getClass();

            for (int i=0; i<2; ++i)
            {
                int attribute = class_.mData.mAttribute[i];
                if (attribute>=0 && attribute<8)
                {
                    creatureStats.getAttribute(attribute).setBase (
                        creatureStats.getAttribute(attribute).getBase() + 10);
                }
            }

            for (int i=0; i<2; ++i)
            {
                int bonus = i==0 ? 10 : 25;

                for (int i2=0; i2<5; ++i2)
                {
                    int index = class_.mData.mSkills[i2][i];

                    if (index>=0 && index<27)
                    {
                        npcStats.getSkill (index).setBase (
                            npcStats.getSkill (index).getBase() + bonus);
                    }
                }
            }

            typedef ESMS::IndexListT<ESM::Skill>::MapType ContainerType;
            const ContainerType& skills = MWBase::Environment::get().getWorld()->getStore().skills.list;

            for (ContainerType::const_iterator iter (skills.begin()); iter!=skills.end(); ++iter)
            {
                if (iter->second.mData.mSpecialization==class_.mData.mSpecialization)
                {
                    int index = iter->first;

                    if (index>=0 && index<27)
                    {
                        npcStats.getSkill (index).setBase (
                            npcStats.getSkill (index).getBase() + 5);
                    }
                }
            }
        }

        // forced update and current value adjustments
        mActors.updateActor (ptr, 0);

        creatureStats.getHealth().setCurrent(creatureStats.getHealth().getModified());
        creatureStats.getMagicka().setCurrent(creatureStats.getMagicka().getModified());
        creatureStats.getFatigue().setCurrent(creatureStats.getFatigue().getModified());
    }


    MechanicsManager::MechanicsManager()
    : mUpdatePlayer (true), mClassSelected (false),
      mRaceSelected (false)
    {
        buildPlayer();
    }

    void MechanicsManager::addActor (const MWWorld::Ptr& ptr)
    {
        mActors.addActor (ptr);
    }

    void MechanicsManager::removeActor (const MWWorld::Ptr& ptr)
    {
        if (ptr==mWatched)
            mWatched = MWWorld::Ptr();

        mActors.removeActor (ptr);
    }

    void MechanicsManager::dropActors (const MWWorld::Ptr::CellStore *cellStore)
    {
        if (!mWatched.isEmpty() && mWatched.getCell()==cellStore)
            mWatched = MWWorld::Ptr();

        mActors.dropActors (cellStore);
    }

    void MechanicsManager::watchActor (const MWWorld::Ptr& ptr)
    {
        mWatched = ptr;
    }

    void MechanicsManager::update (std::vector<std::pair<std::string, Ogre::Vector3> >& movement,
        float duration, bool paused)
    {
        if (!mWatched.isEmpty())
        {
            MWMechanics::CreatureStats& stats =
                MWWorld::Class::get (mWatched).getCreatureStats (mWatched);

            MWMechanics::NpcStats& npcStats =
                MWWorld::Class::get (mWatched).getNpcStats (mWatched);

            static const char *attributeNames[8] =
            {
                "AttribVal1", "AttribVal2", "AttribVal3", "AttribVal4", "AttribVal5",
                "AttribVal6", "AttribVal7", "AttribVal8"
            };

            static const char *dynamicNames[3] =
            {
                "HBar", "MBar", "FBar"
            };

            for (int i=0; i<8; ++i)
            {
                if (stats.getAttribute(i)!=mWatchedCreature.getAttribute(i))
                {
                    mWatchedCreature.setAttribute(i, stats.getAttribute(i));

                    MWBase::Environment::get().getWindowManager()->setValue (attributeNames[i], stats.getAttribute(i));
                }
            }

            if (stats.getHealth() != mWatchedCreature.getHealth()) {
                mWatchedCreature.setHealth(stats.getHealth());
                MWBase::Environment::get().getWindowManager()->setValue(dynamicNames[0], stats.getHealth());
            }
            if (stats.getMagicka() != mWatchedCreature.getMagicka()) {
                mWatchedCreature.setMagicka(stats.getMagicka());
                MWBase::Environment::get().getWindowManager()->setValue(dynamicNames[1], stats.getMagicka());
            }
            if (stats.getFatigue() != mWatchedCreature.getFatigue()) {
                mWatchedCreature.setFatigue(stats.getFatigue());
                MWBase::Environment::get().getWindowManager()->setValue(dynamicNames[2], stats.getFatigue());
            }

            bool update = false;

            //Loop over ESM::Skill::SkillEnum
            for(int i = 0; i < 27; ++i)
            {
                if(npcStats.getSkill (i) != mWatchedNpc.getSkill (i))
                {
                    update = true;
                    mWatchedNpc.getSkill (i) = npcStats.getSkill (i);
                    MWBase::Environment::get().getWindowManager()->setValue((ESM::Skill::SkillEnum)i, npcStats.getSkill (i));
                }
            }

            if (update)
                MWBase::Environment::get().getWindowManager()->updateSkillArea();

            MWBase::Environment::get().getWindowManager()->setValue ("level", stats.getLevel());
        }

        if (mUpdatePlayer)
        {
            // basic player profile; should not change anymore after the creation phase is finished.
            MWBase::Environment::get().getWindowManager()->setValue ("name", MWBase::Environment::get().getWorld()->getPlayer().getName());
            MWBase::Environment::get().getWindowManager()->setValue ("race",
                MWBase::Environment::get().getWorld()->getStore().races.find (MWBase::Environment::get().getWorld()->getPlayer().
                getRace())->mName);
            MWBase::Environment::get().getWindowManager()->setValue ("class",
                MWBase::Environment::get().getWorld()->getPlayer().getClass().mName);
            mUpdatePlayer = false;

            MWBase::WindowManager::SkillList majorSkills (5);
            MWBase::WindowManager::SkillList minorSkills (5);

            for (int i=0; i<5; ++i)
            {
                minorSkills[i] = MWBase::Environment::get().getWorld()->getPlayer().getClass().mData.mSkills[i][0];
                majorSkills[i] = MWBase::Environment::get().getWorld()->getPlayer().getClass().mData.mSkills[i][1];
            }

            MWBase::Environment::get().getWindowManager()->configureSkills (majorSkills, minorSkills);
        }

        mActors.update (movement, duration, paused);
    }

    void MechanicsManager::restoreDynamicStats()
    {
        mActors.restoreDynamicStats ();
    }

    void MechanicsManager::setPlayerName (const std::string& name)
    {
        MWBase::Environment::get().getWorld()->getPlayer().setName (name);
        mUpdatePlayer = true;
    }

    void MechanicsManager::setPlayerRace (const std::string& race, bool male)
    {
        MWBase::Environment::get().getWorld()->getPlayer().setGender (male);
        MWBase::Environment::get().getWorld()->getPlayer().setRace (race);
        mRaceSelected = true;
        buildPlayer();
        mUpdatePlayer = true;
    }

    void MechanicsManager::setPlayerBirthsign (const std::string& id)
    {
        MWBase::Environment::get().getWorld()->getPlayer().setBirthsign (id);
        buildPlayer();
        mUpdatePlayer = true;
    }

    void MechanicsManager::setPlayerClass (const std::string& id)
    {
        MWBase::Environment::get().getWorld()->getPlayer().setClass (*MWBase::Environment::get().getWorld()->getStore().classes.find (id));
        mClassSelected = true;
        buildPlayer();
        mUpdatePlayer = true;
    }

    void MechanicsManager::setPlayerClass (const ESM::Class& class_)
    {
        MWBase::Environment::get().getWorld()->getPlayer().setClass (class_);
        mClassSelected = true;
        buildPlayer();
        mUpdatePlayer = true;
    }
}

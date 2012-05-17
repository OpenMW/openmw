
#include "mechanicsmanager.hpp"

#include <components/esm_store/store.hpp>

#include "../mwgui/window_manager.hpp"

#include "../mwbase/environment.hpp"

#include "../mwworld/class.hpp"
#include "../mwworld/world.hpp"
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
        creatureStats.mLevel = player->npdt52.level;
        creatureStats.mSpells.clear();
        creatureStats.mMagicEffects = MagicEffects();

        for (int i=0; i<27; ++i)
            npcStats.mSkill[i].setBase (player->npdt52.skills[i]);

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
                    case 0: attribute = &race->data.strength; break;
                    case 1: attribute = &race->data.intelligence; break;
                    case 2: attribute = &race->data.willpower; break;
                    case 3: attribute = &race->data.agility; break;
                    case 4: attribute = &race->data.speed; break;
                    case 5: attribute = &race->data.endurance; break;
                    case 6: attribute = &race->data.personality; break;
                    case 7: attribute = &race->data.luck; break;
                }

                creatureStats.mAttributes[i].setBase (
                    static_cast<int> (male ? attribute->male : attribute->female));
            }

            for (int i=0; i<7; ++i)
            {
                int index = race->data.bonus[i].skill;

                if (index>=0 && index<27)
                {
                    npcStats.mSkill[index].setBase (
                        npcStats.mSkill[index].getBase() + race->data.bonus[i].bonus);
                }
            }

            for (std::vector<std::string>::const_iterator iter (race->powers.list.begin());
                iter!=race->powers.list.end(); ++iter)
            {
                creatureStats.mSpells.add (*iter);
            }
        }

        // birthsign
        if (!MWBase::Environment::get().getWorld()->getPlayer().getBirthsign().empty())
        {
            const ESM::BirthSign *sign =
                MWBase::Environment::get().getWorld()->getStore().birthSigns.find (
                MWBase::Environment::get().getWorld()->getPlayer().getBirthsign());

            for (std::vector<std::string>::const_iterator iter (sign->powers.list.begin());
                iter!=sign->powers.list.end(); ++iter)
            {
                creatureStats.mSpells.add (*iter);
            }
        }

        // class
        if (mClassSelected)
        {
            const ESM::Class& class_ = MWBase::Environment::get().getWorld()->getPlayer().getClass();

            for (int i=0; i<2; ++i)
            {
                int attribute = class_.data.attribute[i];
                if (attribute>=0 && attribute<8)
                {
                    creatureStats.mAttributes[attribute].setBase (
                        creatureStats.mAttributes[attribute].getBase() + 10);
                }
            }

            for (int i=0; i<2; ++i)
            {
                int bonus = i==0 ? 10 : 25;

                for (int i2=0; i2<5; ++i2)
                {
                    int index = class_.data.skills[i2][i];

                    if (index>=0 && index<27)
                    {
                        npcStats.mSkill[index].setBase (
                            npcStats.mSkill[index].getBase() + bonus);
                    }
                }
            }

            typedef ESMS::IndexListT<ESM::Skill>::MapType ContainerType;
            const ContainerType& skills = MWBase::Environment::get().getWorld()->getStore().skills.list;

            for (ContainerType::const_iterator iter (skills.begin()); iter!=skills.end(); ++iter)
            {
                if (iter->second.data.specialization==class_.data.specialization)
                {
                    int index = iter->first;

                    if (index>=0 && index<27)
                    {
                        npcStats.mSkill[index].setBase (
                            npcStats.mSkill[index].getBase() + 5);
                    }
                }
            }
        }

        // magic effects
        adjustMagicEffects (ptr);
    }

    void MechanicsManager::adjustMagicEffects (MWWorld::Ptr& creature)
    {
        MWMechanics::CreatureStats& creatureStats =
            MWWorld::Class::get (creature).getCreatureStats (creature);

        MagicEffects now = creatureStats.mSpells.getMagicEffects();

        /// \todo add effects from active spells and equipment

        MagicEffects diff = MagicEffects::diff (creatureStats.mMagicEffects, now);

        creatureStats.mMagicEffects = now;

        // TODO apply diff to other stats
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
                if (stats.mAttributes[i]!=mWatchedCreature.mAttributes[i])
                {
                    mWatchedCreature.mAttributes[i] = stats.mAttributes[i];

                    MWBase::Environment::get().getWindowManager()->setValue (attributeNames[i], stats.mAttributes[i]);
                }
            }

            for (int i=0; i<3; ++i)
            {
                if (stats.mDynamic[i]!=mWatchedCreature.mDynamic[i])
                {
                    mWatchedCreature.mDynamic[i] = stats.mDynamic[i];

                    MWBase::Environment::get().getWindowManager()->setValue (dynamicNames[i], stats.mDynamic[i]);
                }
            }

            bool update = false;

            //Loop over ESM::Skill::SkillEnum
            for(int i = 0; i < 27; ++i)
            {
                if(npcStats.mSkill[i] != mWatchedNpc.mSkill[i])
                {
                    update = true;
                    mWatchedNpc.mSkill[i] = npcStats.mSkill[i];
                    MWBase::Environment::get().getWindowManager()->setValue((ESM::Skill::SkillEnum)i, npcStats.mSkill[i]);
                }
            }

            if (update)
                MWBase::Environment::get().getWindowManager()->updateSkillArea();

            MWBase::Environment::get().getWindowManager()->setValue ("level", stats.mLevel);
        }

        if (mUpdatePlayer)
        {
            // basic player profile; should not change anymore after the creation phase is finished.
            MWBase::Environment::get().getWindowManager()->setValue ("name", MWBase::Environment::get().getWorld()->getPlayer().getName());
            MWBase::Environment::get().getWindowManager()->setValue ("race",
                MWBase::Environment::get().getWorld()->getStore().races.find (MWBase::Environment::get().getWorld()->getPlayer().
                getRace())->name);
            MWBase::Environment::get().getWindowManager()->setValue ("class",
                MWBase::Environment::get().getWorld()->getPlayer().getClass().name);
            mUpdatePlayer = false;

            MWGui::WindowManager::SkillList majorSkills (5);
            MWGui::WindowManager::SkillList minorSkills (5);

            for (int i=0; i<5; ++i)
            {
                minorSkills[i] = MWBase::Environment::get().getWorld()->getPlayer().getClass().data.skills[i][0];
                majorSkills[i] = MWBase::Environment::get().getWorld()->getPlayer().getClass().data.skills[i][1];
            }

            MWBase::Environment::get().getWindowManager()->configureSkills (majorSkills, minorSkills);
        }

        mActors.update (movement, duration, paused);
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

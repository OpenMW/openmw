
#include "mechanicsmanager.hpp"

#include <components/esm_store/store.hpp>

#include "../mwgui/window_manager.hpp"

#include "../mwworld/class.hpp"
#include "../mwworld/environment.hpp"
#include "../mwworld/world.hpp"

namespace MWMechanics
{
    void MechanicsManager::buildPlayer()
    {
        MWWorld::Ptr ptr = mEnvironment.mWorld->getPlayerPos().getPlayer();

        MWMechanics::CreatureStats& creatureStats = MWWorld::Class::get (ptr).getCreatureStats (ptr);
        MWMechanics::NpcStats& npcStats = MWWorld::Class::get (ptr).getNpcStats (ptr);

        const ESM::NPC *player = ptr.get<ESM::NPC>()->base;

        // reset
        creatureStats.mLevel = player->npdt52.level;

        for (int i=0; i<27; ++i)
            npcStats.mSkill[i].setBase (player->npdt52.skills[i]);

        // race
        const ESM::Race *race =
            mEnvironment.mWorld->getStore().races.find (mEnvironment.mWorld->getPlayerPos().getRace());

        bool male = mEnvironment.mWorld->getPlayerPos().isMale();

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

        // birthsign

        // class
        const ESM::Class& class_ = mEnvironment.mWorld->getPlayerPos().getClass();

        for (int i=0; i<2; ++i)
        {
            int attribute = class_.data.attribute[i];
            if (attribute>=0 && attribute<8)
            {
                creatureStats.mAttributes[attribute].setBase (
                    creatureStats.mAttributes[attribute].getBase() + 10);
            }
        }



        // calculate dynamic stats
        int strength = creatureStats.mAttributes[0].getBase();
        int intelligence = creatureStats.mAttributes[1].getBase();
        int willpower = creatureStats.mAttributes[2].getBase();
        int agility = creatureStats.mAttributes[3].getBase();
        int endurance = creatureStats.mAttributes[5].getBase();

        creatureStats.mDynamic[0].setBase (static_cast<int> (0.5 * (strength + endurance)));
        // TODO: calculate factor
        creatureStats.mDynamic[1].setBase (static_cast<int> (intelligence + 1 * intelligence));
        creatureStats.mDynamic[2].setBase (strength+willpower+agility+endurance);

        for (int i=0; i<3; ++i)
            creatureStats.mDynamic[i].setCurrent (creatureStats.mDynamic[i].getModified());
    }

    MechanicsManager::MechanicsManager (MWWorld::Environment& environment)
    : mEnvironment (environment), mUpdatePlayer (true)
    {
        buildPlayer();
    }

    void MechanicsManager::addActor (const MWWorld::Ptr& ptr)
    {
        mActors.insert (ptr);
    }

    void MechanicsManager::removeActor (const MWWorld::Ptr& ptr)
    {
        mActors.erase (ptr);
    }

    void MechanicsManager::dropActors (const MWWorld::Ptr::CellStore *cellStore)
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

    void MechanicsManager::watchActor (const MWWorld::Ptr& ptr)
    {
        mWatched = ptr;
    }

    void MechanicsManager::update()
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

            static const char *skillNames[27] =
            {
                "SkillBlock", "SkillArmorer", "SkillMediumArmor", "SkillHeavyArmor",
                "SkillBluntWeapon", "SkillLongBlade", "SkillAxe", "SkillSpear",
                "SkillAthletics", "SkillEnchant", "SkillDestruction", "SkillAlteration",
                "SkillIllusion", "SkillConjuration", "SkillMysticism", "SkillRestoration",
                "SkillAlchemy", "SkillUnarmored", "SkillSecurity", "SkillSneak",
                "SkillAcrobatics", "SkillLightArmor", "SkillShortBlade", "SkillMarksman",
                "SkillMercantile", "SkillSpeechcraft", "SkillHandToHand",
            };

            for (int i=0; i<8; ++i)
            {
                if (stats.mAttributes[i]!=mWatchedCreature.mAttributes[i])
                {
                    mWatchedCreature.mAttributes[i] = stats.mAttributes[i];

                    mEnvironment.mWindowManager->setValue (attributeNames[i], stats.mAttributes[i]);
                }
            }

            for (int i=0; i<3; ++i)
            {
                if (stats.mDynamic[i]!=mWatchedCreature.mDynamic[i])
                {
                    mWatchedCreature.mDynamic[i] = stats.mDynamic[i];

                    mEnvironment.mWindowManager->setValue (dynamicNames[i], stats.mDynamic[i]);
                }
            }

            bool update = false;

            for (int i=0; i<27; ++i)
            {
                if (npcStats.mSkill[i]!=mWatchedNpc.mSkill[i])
                {
                    update = true;

                    mWatchedNpc.mSkill[i] = npcStats.mSkill[i];

                    mEnvironment.mWindowManager->setValue (skillNames[i], npcStats.mSkill[i]);

                }
            }

            if (update)
                mEnvironment.mWindowManager->updateSkillArea();

            mEnvironment.mWindowManager->setValue ("level", stats.mLevel);
        }

        if (mUpdatePlayer)
        {
            // basic player profile; should not change anymore after the creation phase is finished.
            mEnvironment.mWindowManager->setValue ("name", mEnvironment.mWorld->getPlayerPos().getName());
            mEnvironment.mWindowManager->setValue ("race",
                mEnvironment.mWorld->getStore().races.find (mEnvironment.mWorld->getPlayerPos().
                getRace())->name);
            mEnvironment.mWindowManager->setValue ("class",
                mEnvironment.mWorld->getPlayerPos().getClass().name);
            mUpdatePlayer = false;

            MWGui::WindowManager::SkillList majorSkills (5);
            MWGui::WindowManager::SkillList minorSkills (5);

            for (int i=0; i<5; ++i)
            {
                minorSkills[i] = mEnvironment.mWorld->getPlayerPos().getClass().data.skills[i][0];
                majorSkills[i] = mEnvironment.mWorld->getPlayerPos().getClass().data.skills[i][1];
            }

            mEnvironment.mWindowManager->configureSkills (majorSkills, minorSkills);
        }
    }

    void MechanicsManager::setPlayerName (const std::string& name)
    {
        mEnvironment.mWorld->getPlayerPos().setName (name);
        mUpdatePlayer = true;
    }

    void MechanicsManager::setPlayerRace (const std::string& race, bool male)
    {
        mEnvironment.mWorld->getPlayerPos().setGender (male);
        mEnvironment.mWorld->getPlayerPos().setRace (race);
        buildPlayer();
        mUpdatePlayer = true;
    }

    void MechanicsManager::setPlayerBirthsign (const std::string& id)
    {
        mEnvironment.mWorld->getPlayerPos().setBirthsign (id);
        buildPlayer();
    }

    void MechanicsManager::setPlayerClass (const std::string& id)
    {
        mEnvironment.mWorld->getPlayerPos().setClass (*mEnvironment.mWorld->getStore().classes.find (id));
        buildPlayer();
        mUpdatePlayer = true;
    }

    void MechanicsManager::setPlayerClass (const ESM::Class& class_)
    {
        mEnvironment.mWorld->getPlayerPos().setClass (class_);
        buildPlayer();
        mUpdatePlayer = true;
    }
}

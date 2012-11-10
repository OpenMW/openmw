
#include "mechanicsmanagerimp.hpp"

#include "../mwworld/esmstore.hpp"

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

        const ESM::NPC *player = ptr.get<ESM::NPC>()->mBase;

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
        
        const MWWorld::ESMStore &esmStore =
            MWBase::Environment::get().getWorld()->getStore();

        // race
        if (mRaceSelected)
        {
            const ESM::Race *race =
                esmStore.get<ESM::Race>().find(player->mRace); 

            bool male = (player->mFlags & ESM::NPC::Female) == 0;

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
        const std::string &signId =
            MWBase::Environment::get().getWorld()->getPlayer().getBirthSign();

        if (!signId.empty())
        {
            const ESM::BirthSign *sign =
                esmStore.get<ESM::BirthSign>().find(signId);

            for (std::vector<std::string>::const_iterator iter (sign->mPowers.mList.begin());
                iter!=sign->mPowers.mList.end(); ++iter)
            {
                creatureStats.getSpells().add (*iter);
            }
        }

        // class
        if (mClassSelected)
        {
            const ESM::Class *class_ =
                esmStore.get<ESM::Class>().find(player->mClass);

            for (int i=0; i<2; ++i)
            {
                int attribute = class_->mData.mAttribute[i];
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
                    int index = class_->mData.mSkills[i2][i];

                    if (index>=0 && index<27)
                    {
                        npcStats.getSkill (index).setBase (
                            npcStats.getSkill (index).getBase() + bonus);
                    }
                }
            }

            const MWWorld::Store<ESM::Skill> &skills =
                esmStore.get<ESM::Skill>();

            MWWorld::Store<ESM::Skill>::iterator iter = skills.begin();
            for (; iter != skills.end(); ++iter)
            {
                if (iter->mData.mSpecialization==class_->mData.mSpecialization)
                {
                    int index = iter->mIndex;

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

        for (int i=0; i<2; ++i)
        {
            DynamicStat<float> stat = creatureStats.getDynamic (i);
            stat.setCurrent (stat.getModified());
            creatureStats.setDynamic (i, stat);
        }
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
            MWBase::WindowManager *winMgr =
                MWBase::Environment::get().getWindowManager();
           
            MWBase::World *world = MWBase::Environment::get().getWorld();
            const ESM::NPC *player =
                world->getPlayer().getPlayer().get<ESM::NPC>()->mBase;

            const ESM::Race *race =
                world->getStore().get<ESM::Race>().find(player->mRace);
            const ESM::Class *cls =
                world->getStore().get<ESM::Class>().find(player->mClass);

            winMgr->setValue ("name", player->mName);
            winMgr->setValue ("race", race->mName);
            winMgr->setValue ("class", cls->mName);

            mUpdatePlayer = false;

            MWBase::WindowManager::SkillList majorSkills (5);
            MWBase::WindowManager::SkillList minorSkills (5);

            for (int i=0; i<5; ++i)
            {
                minorSkills[i] = cls->mData.mSkills[i][0];
                majorSkills[i] = cls->mData.mSkills[i][1];
            }

            winMgr->configureSkills (majorSkills, minorSkills);
        }

        mActors.update (movement, duration, paused);
    }

    void MechanicsManager::restoreDynamicStats()
    {
        mActors.restoreDynamicStats ();
    }

    void MechanicsManager::setPlayerName (const std::string& name)
    {
        MWBase::World *world = MWBase::Environment::get().getWorld();

        ESM::NPC player =
            *world->getPlayer().getPlayer().get<ESM::NPC>()->mBase;
        player.mName = name;

        world->createRecord(player);

        mUpdatePlayer = true;
    }

    void MechanicsManager::setPlayerRace (const std::string& race, bool male, const std::string &head, const std::string &hair)
    {
        MWBase::World *world = MWBase::Environment::get().getWorld();

        ESM::NPC player =
            *world->getPlayer().getPlayer().get<ESM::NPC>()->mBase;

        player.mRace = race;
        player.mHead = head;
        player.mHair = hair;
        player.setIsMale(male);

        world->createRecord(player);

        mRaceSelected = true;
        buildPlayer();
        mUpdatePlayer = true;
    }

    void MechanicsManager::setPlayerBirthsign (const std::string& id)
    {
        MWBase::Environment::get().getWorld()->getPlayer().setBirthSign(id);
        buildPlayer();
        mUpdatePlayer = true;
    }

    void MechanicsManager::setPlayerClass (const std::string& id)
    {
        MWBase::World *world = MWBase::Environment::get().getWorld();

        ESM::NPC player =
            *world->getPlayer().getPlayer().get<ESM::NPC>()->mBase;
        player.mClass = id;

        world->createRecord(player);

        mClassSelected = true;
        buildPlayer();
        mUpdatePlayer = true;
    }

    void MechanicsManager::setPlayerClass (const ESM::Class &cls)
    {
        MWBase::World *world = MWBase::Environment::get().getWorld();

        const ESM::Class *ptr = world->createRecord(cls);

        ESM::NPC player =
            *world->getPlayer().getPlayer().get<ESM::NPC>()->mBase;
        player.mClass = ptr->mId;

        world->createRecord(player);

        mClassSelected = true;
        buildPlayer();
        mUpdatePlayer = true;
    }

    std::string toLower (const std::string& name)
    {
        std::string lowerCase;

        std::transform (name.begin(), name.end(), std::back_inserter (lowerCase),
            (int(*)(int)) std::tolower);

        return lowerCase;
    }

    int MechanicsManager::getDerivedDisposition(const MWWorld::Ptr& ptr)
    {
        MWMechanics::NpcStats npcSkill = MWWorld::Class::get(ptr).getNpcStats(ptr);
        float x = npcSkill.getDisposition();

        MWWorld::LiveCellRef<ESM::NPC>* npc = ptr.get<ESM::NPC>();
        MWWorld::Ptr playerPtr = MWBase::Environment::get().getWorld()->getPlayer().getPlayer();
        MWWorld::LiveCellRef<ESM::NPC>* player = playerPtr.get<ESM::NPC>();
        MWMechanics::CreatureStats playerStats = MWWorld::Class::get(playerPtr).getCreatureStats(playerPtr);
        MWMechanics::NpcStats playerNpcStats = MWWorld::Class::get(playerPtr).getNpcStats(playerPtr);

        if (toLower(npc->mBase->mRace) == toLower(player->mBase->mRace)) x += MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>().find("fDispRaceMod")->getFloat();

        x += MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>().find("fDispPersonalityMult")->getFloat()
            * (playerStats.getAttribute(ESM::Attribute::Personality).getModified() - MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>().find("fDispPersonalityBase")->getFloat());

        float reaction = 0;
        int rank = 0;
        std::string npcFaction = "";
        if(!npcSkill.getFactionRanks().empty()) npcFaction = npcSkill.getFactionRanks().begin()->first;

        if (playerNpcStats.getFactionRanks().find(toLower(npcFaction)) != playerNpcStats.getFactionRanks().end())
        {
            for(std::vector<ESM::Faction::Reaction>::const_iterator it = MWBase::Environment::get().getWorld()->getStore().get<ESM::Faction>().find(toLower(npcFaction))->mReactions.begin();
                it != MWBase::Environment::get().getWorld()->getStore().get<ESM::Faction>().find(toLower(npcFaction))->mReactions.end(); it++)
            {
                if(toLower(it->mFaction) == toLower(npcFaction)) reaction = it->mReaction;
            }
            rank = playerNpcStats.getFactionRanks().find(toLower(npcFaction))->second;
        }
        else if (npcFaction != "")
        {
            for(std::vector<ESM::Faction::Reaction>::const_iterator it = MWBase::Environment::get().getWorld()->getStore().get<ESM::Faction>().find(toLower(npcFaction))->mReactions.begin();
                it != MWBase::Environment::get().getWorld()->getStore().get<ESM::Faction>().find(toLower(npcFaction))->mReactions.end();it++)
            {
                if(playerNpcStats.getFactionRanks().find(toLower(it->mFaction)) != playerNpcStats.getFactionRanks().end() )
                {
                    if(it->mReaction<reaction) reaction = it->mReaction;
                }
            }
            rank = 0;
        }
        else
        {
            reaction = 0;
            rank = 0;
        }
        x += (MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>().find("fDispFactionRankMult")->getFloat() * rank
            + MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>().find("fDispFactionRankBase")->getFloat())
            * MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>().find("fDispFactionMod")->getFloat() * reaction;

        x -= MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>().find("fDispCrimeMod")->getFloat() * playerNpcStats.getBounty();
        if (playerStats.hasCommonDisease() || playerStats.hasBlightDisease())
            x += MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>().find("fDispDiseaseMod")->getFloat();

        if (playerNpcStats.getDrawState() == MWMechanics::DrawState_::DrawState_Weapon)
            x += MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>().find("fDispWeaponDrawn")->getFloat();

        int effective_disposition = std::max(0,std::min(int(x),100));//, normally clamped to [0..100] when used
        return effective_disposition;
    }

    int MechanicsManager::getBarterOffer(const MWWorld::Ptr& ptr,int basePrice, bool buying)
    {
        if (ptr.getTypeName() == typeid(ESM::Creature).name())
            return basePrice;

        MWMechanics::NpcStats sellerSkill = MWWorld::Class::get(ptr).getNpcStats(ptr);
        MWMechanics::CreatureStats sellerStats = MWWorld::Class::get(ptr).getCreatureStats(ptr); 

        MWWorld::Ptr playerPtr = MWBase::Environment::get().getWorld()->getPlayer().getPlayer();
        MWMechanics::NpcStats playerSkill = MWWorld::Class::get(playerPtr).getNpcStats(playerPtr);
        MWMechanics::CreatureStats playerStats = MWWorld::Class::get(playerPtr).getCreatureStats(playerPtr);

        int clampedDisposition = std::min(getDerivedDisposition(ptr),100);
        float a = std::min(playerSkill.getSkill(ESM::Skill::Mercantile).getModified(), 100.f);
        float b = std::min(0.1f * playerStats.getAttribute(ESM::Attribute::Luck).getModified(), 10.f);
        float c = std::min(0.2f * playerStats.getAttribute(ESM::Attribute::Personality).getModified(), 10.f);
        float d = std::min(sellerSkill.getSkill(ESM::Skill::Mercantile).getModified(), 100.f);
        float e = std::min(0.1f * sellerStats.getAttribute(ESM::Attribute::Luck).getModified(), 10.f);
        float f = std::min(0.2f * sellerStats.getAttribute(ESM::Attribute::Personality).getModified(), 10.f);
 
        float pcTerm = (clampedDisposition - 50 + a + b + c) * playerStats.getFatigueTerm();
        float npcTerm = (d + e + f) * sellerStats.getFatigueTerm();
        float buyTerm = 0.01 * (100 - 0.5 * (pcTerm - npcTerm));
        float sellTerm = 0.01 * (50 - 0.5 * (npcTerm - pcTerm));

        float x; 
        if(buying) x = buyTerm;
        else x = std::min(buyTerm, sellTerm);
        int offerPrice;
        if (x < 1) offerPrice = int(x * basePrice);
        if (x >= 1) offerPrice = basePrice + int((x - 1) * basePrice);
        offerPrice = std::max(1, offerPrice);
        return offerPrice;
    }

    int MechanicsManager::countDeaths (const std::string& id) const
    {
        return mActors.countDeaths (id);
    }
}

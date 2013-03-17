
#include "mechanicsmanagerimp.hpp"

#include "../mwworld/esmstore.hpp"
#include "../mwworld/inventorystore.hpp"

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwbase/windowmanager.hpp"
#include "../mwbase/dialoguemanager.hpp"

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

    void MechanicsManager::add(const MWWorld::Ptr& ptr)
    {
        if(ptr.getTypeName() == typeid(ESM::Activator).name())
            mActivators.addActivator(ptr);
        else
            mActors.addActor(ptr);
    }

    void MechanicsManager::remove(const MWWorld::Ptr& ptr)
    {
        if(ptr == mWatched)
            mWatched = MWWorld::Ptr();
        mActors.removeActor(ptr);
        mActivators.removeActivator(ptr);
    }

    void MechanicsManager::updateCell(const MWWorld::Ptr &old, const MWWorld::Ptr &ptr)
    {
        if(ptr.getTypeName() == typeid(ESM::Activator).name())
            mActivators.updateActivator(old, ptr);
        else
            mActors.updateActor(old, ptr);
    }


    void MechanicsManager::drop(const MWWorld::CellStore *cellStore)
    {
        if(!mWatched.isEmpty() && mWatched.getCell() == cellStore)
            mWatched = MWWorld::Ptr();

        mActors.dropActors(cellStore);
        mActivators.dropActivators(cellStore);
    }


    void MechanicsManager::watchActor(const MWWorld::Ptr& ptr)
    {
        mWatched = ptr;
    }

    void MechanicsManager::update(float duration, bool paused)
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

            // HACK? The player has been changed, so a new Animation object may
            // have been made for them. Make sure they're properly updated.
            MWWorld::Ptr ptr = MWBase::Environment::get().getWorld()->getPlayer().getPlayer();
            mActors.removeActor(ptr);
            mActors.addActor(ptr);
        }

        mActors.update(duration, paused);
        mActivators.update(duration, paused);
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

    int MechanicsManager::getDerivedDisposition(const MWWorld::Ptr& ptr)
    {
        MWMechanics::NpcStats npcSkill = MWWorld::Class::get(ptr).getNpcStats(ptr);
        float x = npcSkill.getBaseDisposition();

        MWWorld::LiveCellRef<ESM::NPC>* npc = ptr.get<ESM::NPC>();
        MWWorld::Ptr playerPtr = MWBase::Environment::get().getWorld()->getPlayer().getPlayer();
        MWWorld::LiveCellRef<ESM::NPC>* player = playerPtr.get<ESM::NPC>();
        MWMechanics::CreatureStats playerStats = MWWorld::Class::get(playerPtr).getCreatureStats(playerPtr);
        MWMechanics::NpcStats playerNpcStats = MWWorld::Class::get(playerPtr).getNpcStats(playerPtr);

        if (Misc::StringUtils::lowerCase(npc->mBase->mRace) == Misc::StringUtils::lowerCase(player->mBase->mRace)) x += MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>().find("fDispRaceMod")->getFloat();

        x += MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>().find("fDispPersonalityMult")->getFloat()
            * (playerStats.getAttribute(ESM::Attribute::Personality).getModified() - MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>().find("fDispPersonalityBase")->getFloat());

        float reaction = 0;
        int rank = 0;
        std::string npcFaction = "";
        if(!npcSkill.getFactionRanks().empty()) npcFaction = npcSkill.getFactionRanks().begin()->first;

        if (playerNpcStats.getFactionRanks().find(Misc::StringUtils::lowerCase(npcFaction)) != playerNpcStats.getFactionRanks().end())
        {
            for(std::vector<ESM::Faction::Reaction>::const_iterator it = MWBase::Environment::get().getWorld()->getStore().get<ESM::Faction>().find(Misc::StringUtils::lowerCase(npcFaction))->mReactions.begin();
                it != MWBase::Environment::get().getWorld()->getStore().get<ESM::Faction>().find(Misc::StringUtils::lowerCase(npcFaction))->mReactions.end(); it++)
            {
                if(Misc::StringUtils::lowerCase(it->mFaction) == Misc::StringUtils::lowerCase(npcFaction)) reaction = it->mReaction;
            }
            rank = playerNpcStats.getFactionRanks().find(Misc::StringUtils::lowerCase(npcFaction))->second;
        }
        else if (npcFaction != "")
        {
            for(std::vector<ESM::Faction::Reaction>::const_iterator it = MWBase::Environment::get().getWorld()->getStore().get<ESM::Faction>().find(Misc::StringUtils::lowerCase(npcFaction))->mReactions.begin();
                it != MWBase::Environment::get().getWorld()->getStore().get<ESM::Faction>().find(Misc::StringUtils::lowerCase(npcFaction))->mReactions.end();it++)
            {
                if(playerNpcStats.getFactionRanks().find(Misc::StringUtils::lowerCase(it->mFaction)) != playerNpcStats.getFactionRanks().end() )
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

        if (playerNpcStats.getDrawState() == MWMechanics::DrawState_Weapon)
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

        // I suppose the temporary disposition change _has_ to be considered here,
        // otherwise one would get different prices when exiting and re-entering the dialogue window...
        int clampedDisposition = std::max(0, std::min(getDerivedDisposition(ptr)
            + MWBase::Environment::get().getDialogueManager()->getTemporaryDispositionChange(),100));
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
        if (x < 1)
            offerPrice = int(x * basePrice);
        else
            offerPrice = basePrice + int((x - 1) * basePrice);
        offerPrice = std::max(1, offerPrice);
        return offerPrice;
    }

    int MechanicsManager::countDeaths (const std::string& id) const
    {
        return mActors.countDeaths (id);
    }


    void MechanicsManager::getPersuasionDispositionChange (const MWWorld::Ptr& npc, PersuasionType type,
        float currentTemporaryDispositionDelta, bool& success, float& tempChange, float& permChange)
    {
        const MWWorld::Store<ESM::GameSetting> &gmst =
            MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>();

        MWWorld::Ptr playerPtr = MWBase::Environment::get().getWorld()->getPlayer().getPlayer();
        MWMechanics::NpcStats playerSkill = MWWorld::Class::get(playerPtr).getNpcStats(playerPtr);
        MWMechanics::CreatureStats playerStats = MWWorld::Class::get(playerPtr).getCreatureStats(playerPtr);

        MWMechanics::NpcStats npcSkill = MWWorld::Class::get(npc).getNpcStats(npc);
        MWMechanics::CreatureStats npcStats = MWWorld::Class::get(npc).getCreatureStats(npc);


        float persTerm = playerStats.getAttribute(ESM::Attribute::Personality).getModified()
                            / gmst.find("fPersonalityMod")->getFloat();

        float luckTerm = playerStats.getAttribute(ESM::Attribute::Luck).getModified()
                            / gmst.find("fLuckMod")->getFloat();

        float repTerm = playerSkill.getReputation() * gmst.find("fReputationMod")->getFloat();

        float levelTerm = playerStats.getLevel() * gmst.find("fLevelMod")->getFloat();

        float fatigueTerm = playerStats.getFatigueTerm();

        float playerRating1 = (repTerm + luckTerm + persTerm + playerSkill.getSkill(ESM::Skill::Speechcraft).getModified()) * fatigueTerm;
        float playerRating2 = playerRating1 + levelTerm;
        float playerRating3 = (playerSkill.getSkill(ESM::Skill::Mercantile).getModified() + luckTerm + persTerm) * fatigueTerm;

        float npcRating1 = (repTerm + luckTerm + persTerm + playerSkill.getSkill(ESM::Skill::Speechcraft).getModified()) * fatigueTerm;
        float npcRating2 = (levelTerm + repTerm + luckTerm + persTerm + npcSkill.getSkill(ESM::Skill::Speechcraft).getModified()) * fatigueTerm;
        float npcRating3 = (playerSkill.getSkill(ESM::Skill::Mercantile).getModified() + repTerm + luckTerm + persTerm) * fatigueTerm;

        int currentDisposition = std::min(100, std::max(0, int(getDerivedDisposition(npc) + currentTemporaryDispositionDelta)));

        float d = 1 - 0.02 * abs(currentDisposition - 50);
        float target1 = d * (playerRating1 - npcRating1 + 50);
        float target2 = d * (playerRating2 - npcRating2 + 50);

        float bribeMod;
        if (type == PT_Bribe10) bribeMod = gmst.find("fBribe10Mod")->getFloat();
        if (type == PT_Bribe100) bribeMod = gmst.find("fBribe100Mod")->getFloat();
        else bribeMod = gmst.find("fBribe1000Mod")->getFloat();

        float target3 = d * (playerRating3 - npcRating3 + 50) + bribeMod;

        float iPerMinChance = gmst.find("iPerMinChance")->getInt();
        float iPerMinChange = gmst.find("iPerMinChange")->getInt();
        float fPerDieRollMult = gmst.find("fPerDieRollMult")->getFloat();
        float fPerTempMult = gmst.find("fPerTempMult")->getFloat();

        float x = 0;
        float y = 0;

        float roll = static_cast<float> (std::rand()) / RAND_MAX * 100;

        if (type == PT_Admire)
        {
            target1 = std::max(iPerMinChance, target1);
            success = (roll <= target1);
            float c = int(fPerDieRollMult * (target1 - roll));
            x = success ? std::max(iPerMinChange, c) : c;
        }
        else if (type == PT_Intimidate)
        {
            target2 = std::max(iPerMinChance, target2);

            success =  (roll <= target2);

            float r;
            if (roll != target2)
                r = int(target2 - roll);
            else
                r = 1;

            if (roll <= target2)
            {
                float s = int(r * fPerDieRollMult * fPerTempMult);

                npcStats.setAiSetting (2, std::max(0, std::min(100, npcStats.getAiSetting (2) + int(std::max(iPerMinChange, s)))));
                npcStats.setAiSetting (1, std::max(0, std::min(100, npcStats.getAiSetting (1) + int(std::min(-iPerMinChange, -s)))));
            }

            float c = -std::abs(int(r * fPerDieRollMult));
            if (success)
            {
                if (std::abs(c) < iPerMinChange)
                {
                    x = 0;
                    y = -iPerMinChange;
                }
                else
                {
                    x = -int(c * fPerTempMult);
                    y = c;
                }
            }
            else
            {
                x = int(c * fPerTempMult);
                y = c;
            }
        }
        else if (type == PT_Taunt)
        {
            target1 = std::max(iPerMinChance, target1);
            success = (roll <= target1);

            float c = std::abs(int(target1 - roll));

            if (roll <= target1)
            {
                float s = c * fPerDieRollMult * fPerTempMult;

                npcStats.setAiSetting (2, std::max(0, std::min(100, npcStats.getAiSetting (2) + std::min(-int(iPerMinChange), int(-s)))));
                npcStats.setAiSetting (1, std::max(0, std::min(100, npcStats.getAiSetting (1) + std::max(int(iPerMinChange), int(s)))));
            }
            x = int(-c * fPerDieRollMult);

            if (success && std::abs(x) < iPerMinChange)
                x = -iPerMinChange;
        }
        else // Bribe
        {
            target3 = std::max(iPerMinChance, target3);
            success = (roll <= target3);
            float c = int((target3 - roll) * fPerDieRollMult);

            x = success ? std::max(iPerMinChange, c) : c;
        }

        tempChange = type == PT_Intimidate ? x : int(x * fPerTempMult);


        float cappedDispositionChange = tempChange;
        if (currentDisposition + tempChange > 100.f)
            cappedDispositionChange = 100 - currentDisposition;
        if (currentDisposition + tempChange < 0.f)
            cappedDispositionChange = -currentDisposition;

        permChange = int(cappedDispositionChange / fPerTempMult);
        if (type == PT_Intimidate)
        {
            permChange = success ? -int(cappedDispositionChange/ fPerTempMult) : y;
        }
    }

    void MechanicsManager::playAnimationGroup(const MWWorld::Ptr& ptr, const std::string& groupName, int mode, int number)
    {
        if(ptr.getTypeName() == typeid(ESM::Activator).name())
            mActivators.playAnimationGroup(ptr, groupName, mode, number);
        else
            mActors.playAnimationGroup(ptr, groupName, mode, number);
    }
    void MechanicsManager::skipAnimation(const MWWorld::Ptr& ptr)
    {
        if(ptr.getTypeName() == typeid(ESM::Activator).name())
            mActivators.skipAnimation(ptr);
        else
            mActors.skipAnimation(ptr);
    }

}

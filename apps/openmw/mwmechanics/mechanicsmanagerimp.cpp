
#include "mechanicsmanagerimp.hpp"
#include "npcstats.hpp"

#include "../mwworld/esmstore.hpp"
#include "../mwworld/inventorystore.hpp"

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwbase/windowmanager.hpp"
#include "../mwbase/dialoguemanager.hpp"

#include "../mwworld/class.hpp"
#include "../mwworld/player.hpp"

#include <OgreSceneNode.h>

#include "spellcasting.hpp"

namespace
{
    /// @return is \a ptr allowed to take/use \a item or is it a crime?
    bool isAllowedToUse (const MWWorld::Ptr& ptr, const MWWorld::Ptr& item, MWWorld::Ptr& victim)
    {
        const std::string& owner = item.getCellRef().mOwner;
        bool isOwned = !owner.empty();

        const std::string& faction = item.getCellRef().mFaction;
        bool isFactionOwned = false;
        if (!faction.empty())
        {
            const std::map<std::string, int>& factions = ptr.getClass().getNpcStats(ptr).getFactionRanks();
            if (factions.find(Misc::StringUtils::lowerCase(faction)) == factions.end())
                isFactionOwned = true;
        }

        if (!item.getCellRef().mOwner.empty())
            victim = MWBase::Environment::get().getWorld()->searchPtr(item.getCellRef().mOwner, true);

        return (!isOwned && !isFactionOwned);
    }
}

namespace MWMechanics
{
    void MechanicsManager::buildPlayer()
    {
        MWWorld::Ptr ptr = MWBase::Environment::get().getWorld()->getPlayerPtr();

        MWMechanics::CreatureStats& creatureStats = MWWorld::Class::get (ptr).getCreatureStats (ptr);
        MWMechanics::NpcStats& npcStats = MWWorld::Class::get (ptr).getNpcStats (ptr);

        const ESM::NPC *player = ptr.get<ESM::NPC>()->mBase;

        // reset
        creatureStats.setLevel(player->mNpdt52.mLevel);
        creatureStats.getSpells().clear();
        creatureStats.setMagicEffects(MagicEffects());

        for (int i=0; i<27; ++i)
            npcStats.getSkill (i).setBase (player->mNpdt52.mSkills[i]);

        creatureStats.setAttribute(ESM::Attribute::Strength, player->mNpdt52.mStrength);
        creatureStats.setAttribute(ESM::Attribute::Intelligence, player->mNpdt52.mIntelligence);
        creatureStats.setAttribute(ESM::Attribute::Willpower, player->mNpdt52.mWillpower);
        creatureStats.setAttribute(ESM::Attribute::Agility, player->mNpdt52.mAgility);
        creatureStats.setAttribute(ESM::Attribute::Speed, player->mNpdt52.mSpeed);
        creatureStats.setAttribute(ESM::Attribute::Endurance, player->mNpdt52.mEndurance);
        creatureStats.setAttribute(ESM::Attribute::Personality, player->mNpdt52.mPersonality);
        creatureStats.setAttribute(ESM::Attribute::Luck, player->mNpdt52.mLuck);
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
                const ESM::Race::MaleFemale& attribute = race->mData.mAttributeValues[i];

                creatureStats.setAttribute(i, male ? attribute.mMale : attribute.mFemale);
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
                    creatureStats.setAttribute(attribute,
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

                    if (i==1)
                    {
                        // Major skill - add starting spells for this skill if existing
                        const MWWorld::ESMStore& store = MWBase::Environment::get().getWorld()->getStore();
                        MWWorld::Store<ESM::Spell>::iterator it = store.get<ESM::Spell>().begin();
                        for (; it != store.get<ESM::Spell>().end(); ++it)
                        {
                            if (it->mData.mFlags & ESM::Spell::F_PCStart
                                    && spellSchoolToSkill(getSpellSchool(&*it, ptr)) == index)
                                creatureStats.getSpells().add(it->mId);
                        }
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

        for (int i=0; i<3; ++i)
        {
            DynamicStat<float> stat = creatureStats.getDynamic (i);
            stat.setCurrent (stat.getModified());
            creatureStats.setDynamic (i, stat);
        }

        // auto-equip again. we need this for when the race is changed to a beast race and shoes are no longer equippable
        MWWorld::InventoryStore& invStore = ptr.getClass().getInventoryStore(ptr);
        for (int i=0; i<MWWorld::InventoryStore::Slots; ++i)
            invStore.unequipAll(ptr);
        invStore.autoEquip(ptr);
    }

    MechanicsManager::MechanicsManager()
    : mUpdatePlayer (true), mClassSelected (false),
      mRaceSelected (false), mAI(true)
    {
        //buildPlayer no longer here, needs to be done explicitely after all subsystems are up and running
    }

    void MechanicsManager::add(const MWWorld::Ptr& ptr)
    {
        if(MWWorld::Class::get(ptr).isActor())
            mActors.addActor(ptr);
        else
            mObjects.addObject(ptr);
    }

    void MechanicsManager::remove(const MWWorld::Ptr& ptr)
    {
        if(ptr == mWatched)
            mWatched = MWWorld::Ptr();
        mActors.removeActor(ptr);
        mObjects.removeObject(ptr);
    }

    void MechanicsManager::updateCell(const MWWorld::Ptr &old, const MWWorld::Ptr &ptr)
    {
        if(old == mWatched)
            mWatched = ptr;

        if(MWWorld::Class::get(ptr).isActor())
            mActors.updateActor(old, ptr);
        else
            mObjects.updateObject(old, ptr);
    }


    void MechanicsManager::drop(const MWWorld::CellStore *cellStore)
    {
        mActors.dropActors(cellStore, mWatched);
        mObjects.dropObjects(cellStore);
    }


    void MechanicsManager::watchActor(const MWWorld::Ptr& ptr)
    {
        mWatched = ptr;
    }

    void MechanicsManager::advanceTime (float duration)
    {
        // Uses ingame time, but scaled to real time
        duration /= MWBase::Environment::get().getWorld()->getTimeScaleFactor();
        MWWorld::Ptr player = MWBase::Environment::get().getWorld()->getPlayerPtr();
        player.getClass().getInventoryStore(player).rechargeItems(duration);
    }

    void MechanicsManager::update(float duration, bool paused)
    {
        if(!mWatched.isEmpty())
        {
            MWBase::WindowManager *winMgr = MWBase::Environment::get().getWindowManager();
            const MWMechanics::NpcStats &stats = mWatched.getClass().getNpcStats(mWatched);
            for(int i = 0;i < ESM::Attribute::Length;++i)
            {
                if(stats.getAttribute(i) != mWatchedStats.getAttribute(i))
                {
                    std::stringstream attrname;
                    attrname << "AttribVal"<<(i+1);

                    mWatchedStats.setAttribute(i, stats.getAttribute(i));
                    winMgr->setValue(attrname.str(), stats.getAttribute(i));
                }
            }

            if(stats.getHealth() != mWatchedStats.getHealth())
            {
                static const std::string hbar("HBar");
                mWatchedStats.setHealth(stats.getHealth());
                winMgr->setValue(hbar, stats.getHealth());
            }
            if(stats.getMagicka() != mWatchedStats.getMagicka())
            {
                static const std::string mbar("MBar");
                mWatchedStats.setMagicka(stats.getMagicka());
                winMgr->setValue(mbar, stats.getMagicka());
            }
            if(stats.getFatigue() != mWatchedStats.getFatigue())
            {
                static const std::string fbar("FBar");
                mWatchedStats.setFatigue(stats.getFatigue());
                winMgr->setValue(fbar, stats.getFatigue());
            }

            if(stats.getTimeToStartDrowning() != mWatchedStats.getTimeToStartDrowning())
            {
                const float fHoldBreathTime = MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>()
                        .find("fHoldBreathTime")->getFloat();
                mWatchedStats.setTimeToStartDrowning(stats.getTimeToStartDrowning());
                if(stats.getTimeToStartDrowning() >= fHoldBreathTime)
                    winMgr->setDrowningBarVisibility(false);
                else
                {
                    winMgr->setDrowningBarVisibility(true);
                    winMgr->setDrowningTimeLeft(stats.getTimeToStartDrowning(), fHoldBreathTime);
                }
            }

            bool update = false;

            //Loop over ESM::Skill::SkillEnum
            for(int i = 0; i < ESM::Skill::Length; ++i)
            {
                if(stats.getSkill(i) != mWatchedStats.getSkill(i))
                {
                    update = true;
                    mWatchedStats.getSkill(i) = stats.getSkill(i);
                    winMgr->setValue((ESM::Skill::SkillEnum)i, stats.getSkill(i));
                }
            }

            if(update)
                winMgr->updateSkillArea();

            winMgr->setValue("level", stats.getLevel());

            // Update the equipped weapon icon
            MWWorld::InventoryStore& inv = mWatched.getClass().getInventoryStore(mWatched);
            MWWorld::ContainerStoreIterator weapon = inv.getSlot(MWWorld::InventoryStore::Slot_CarriedRight);
            if (weapon == inv.end())
                winMgr->unsetSelectedWeapon();
            else
                winMgr->setSelectedWeapon(*weapon);

            // Update the selected spell icon
            MWWorld::ContainerStoreIterator enchantItem = inv.getSelectedEnchantItem();
            if (enchantItem != inv.end())
                winMgr->setSelectedEnchantItem(*enchantItem);
            else if (winMgr->getSelectedSpell() == "")
                winMgr->unsetSelectedSpell();
        }

        if (mUpdatePlayer)
        {
            MWBase::World *world = MWBase::Environment::get().getWorld();

            // basic player profile; should not change anymore after the creation phase is finished.
            MWBase::WindowManager *winMgr =
                MWBase::Environment::get().getWindowManager();

            const ESM::NPC *player =
                world->getPlayerPtr().get<ESM::NPC>()->mBase;

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
            MWWorld::Ptr ptr = MWBase::Environment::get().getWorld()->getPlayerPtr();
            mActors.removeActor(ptr);
            mActors.addActor(ptr);
        }

        mActors.update(duration, paused);
        mObjects.update(duration, paused);
    }

    void MechanicsManager::rest(bool sleep)
    {
        mActors.restoreDynamicStats (sleep);
    }

    int MechanicsManager::getHoursToRest() const
    {
        return mActors.getHoursToRest(mWatched);
    }

    void MechanicsManager::setPlayerName (const std::string& name)
    {
        MWBase::World *world = MWBase::Environment::get().getWorld();

        ESM::NPC player =
            *world->getPlayerPtr().get<ESM::NPC>()->mBase;
        player.mName = name;

        world->createRecord(player);

        mUpdatePlayer = true;
    }

    void MechanicsManager::setPlayerRace (const std::string& race, bool male, const std::string &head, const std::string &hair)
    {
        MWBase::World *world = MWBase::Environment::get().getWorld();

        ESM::NPC player =
            *world->getPlayerPtr().get<ESM::NPC>()->mBase;

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
            *world->getPlayerPtr().get<ESM::NPC>()->mBase;
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
            *world->getPlayerPtr().get<ESM::NPC>()->mBase;
        player.mClass = ptr->mId;

        world->createRecord(player);

        mClassSelected = true;
        buildPlayer();
        mUpdatePlayer = true;
    }

    int MechanicsManager::getDerivedDisposition(const MWWorld::Ptr& ptr)
    {
        const MWMechanics::NpcStats& npcSkill = MWWorld::Class::get(ptr).getNpcStats(ptr);
        float x = npcSkill.getBaseDisposition();

        MWWorld::LiveCellRef<ESM::NPC>* npc = ptr.get<ESM::NPC>();
        MWWorld::Ptr playerPtr = MWBase::Environment::get().getWorld()->getPlayerPtr();
        MWWorld::LiveCellRef<ESM::NPC>* player = playerPtr.get<ESM::NPC>();
        const MWMechanics::NpcStats &playerStats = MWWorld::Class::get(playerPtr).getNpcStats(playerPtr);

        if (Misc::StringUtils::ciEqual(npc->mBase->mRace, player->mBase->mRace))
            x += MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>().find("fDispRaceMod")->getFloat();

        x += MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>().find("fDispPersonalityMult")->getFloat()
            * (playerStats.getAttribute(ESM::Attribute::Personality).getModified() - MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>().find("fDispPersonalityBase")->getFloat());

        float reaction = 0;
        int rank = 0;
        std::string npcFaction = "";
        if(!npcSkill.getFactionRanks().empty()) npcFaction = npcSkill.getFactionRanks().begin()->first;

        Misc::StringUtils::toLower(npcFaction);

        if (playerStats.getFactionRanks().find(npcFaction) != playerStats.getFactionRanks().end())
        {
            for(std::vector<ESM::Faction::Reaction>::const_iterator it = MWBase::Environment::get().getWorld()->getStore().get<ESM::Faction>().find(npcFaction)->mReactions.begin();
                it != MWBase::Environment::get().getWorld()->getStore().get<ESM::Faction>().find(npcFaction)->mReactions.end(); ++it)
            {
                if(Misc::StringUtils::ciEqual(it->mFaction, npcFaction)
                        && !playerStats.getExpelled(it->mFaction))
                    reaction = it->mReaction;
            }
            rank = playerStats.getFactionRanks().find(npcFaction)->second;
        }
        else if (npcFaction != "")
        {
            for(std::vector<ESM::Faction::Reaction>::const_iterator it = MWBase::Environment::get().getWorld()->getStore().get<ESM::Faction>().find(npcFaction)->mReactions.begin();
                it != MWBase::Environment::get().getWorld()->getStore().get<ESM::Faction>().find(npcFaction)->mReactions.end();++it)
            {
                if(playerStats.getFactionRanks().find(Misc::StringUtils::lowerCase(it->mFaction)) != playerStats.getFactionRanks().end() )
                {
                    if(it->mReaction < reaction)
                        reaction = it->mReaction;
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

        x -= MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>().find("fDispCrimeMod")->getFloat() * playerStats.getBounty();
        if (playerStats.hasCommonDisease() || playerStats.hasBlightDisease())
            x += MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>().find("fDispDiseaseMod")->getFloat();

        if (playerStats.getDrawState() == MWMechanics::DrawState_Weapon)
            x += MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>().find("fDispWeaponDrawn")->getFloat();

        x += ptr.getClass().getCreatureStats(ptr).getMagicEffects().get(ESM::MagicEffect::Charm).mMagnitude;

        int effective_disposition = std::max(0,std::min(int(x),100));//, normally clamped to [0..100] when used
        return effective_disposition;
    }

    int MechanicsManager::getBarterOffer(const MWWorld::Ptr& ptr,int basePrice, bool buying)
    {
        if (ptr.getTypeName() == typeid(ESM::Creature).name())
            return basePrice;

        const MWMechanics::NpcStats &sellerStats = MWWorld::Class::get(ptr).getNpcStats(ptr);

        MWWorld::Ptr playerPtr = MWBase::Environment::get().getWorld()->getPlayerPtr();
        const MWMechanics::NpcStats &playerStats = MWWorld::Class::get(playerPtr).getNpcStats(playerPtr);

        // I suppose the temporary disposition change _has_ to be considered here,
        // otherwise one would get different prices when exiting and re-entering the dialogue window...
        int clampedDisposition = std::max(0, std::min(getDerivedDisposition(ptr)
            + MWBase::Environment::get().getDialogueManager()->getTemporaryDispositionChange(),100));
        float a = std::min(playerStats.getSkill(ESM::Skill::Mercantile).getModified(), 100);
        float b = std::min(0.1f * playerStats.getAttribute(ESM::Attribute::Luck).getModified(), 10.f);
        float c = std::min(0.2f * playerStats.getAttribute(ESM::Attribute::Personality).getModified(), 10.f);
        float d = std::min(sellerStats.getSkill(ESM::Skill::Mercantile).getModified(), 100);
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

        MWMechanics::NpcStats& npcStats = MWWorld::Class::get(npc).getNpcStats(npc);

        MWWorld::Ptr playerPtr = MWBase::Environment::get().getWorld()->getPlayerPtr();
        const MWMechanics::NpcStats &playerStats = MWWorld::Class::get(playerPtr).getNpcStats(playerPtr);

        float persTerm = playerStats.getAttribute(ESM::Attribute::Personality).getModified()
                            / gmst.find("fPersonalityMod")->getFloat();

        float luckTerm = playerStats.getAttribute(ESM::Attribute::Luck).getModified()
                            / gmst.find("fLuckMod")->getFloat();

        float repTerm = playerStats.getReputation() * gmst.find("fReputationMod")->getFloat();
        float levelTerm = playerStats.getLevel() * gmst.find("fLevelMod")->getFloat();

        float fatigueTerm = playerStats.getFatigueTerm();

        float playerRating1 = (repTerm + luckTerm + persTerm + playerStats.getSkill(ESM::Skill::Speechcraft).getModified()) * fatigueTerm;
        float playerRating2 = playerRating1 + levelTerm;
        float playerRating3 = (playerStats.getSkill(ESM::Skill::Mercantile).getModified() + luckTerm + persTerm) * fatigueTerm;

        float npcRating1 = (repTerm + luckTerm + persTerm + playerStats.getSkill(ESM::Skill::Speechcraft).getModified()) * fatigueTerm;
        float npcRating2 = (levelTerm + repTerm + luckTerm + persTerm + npcStats.getSkill(ESM::Skill::Speechcraft).getModified()) * fatigueTerm;
        float npcRating3 = (playerStats.getSkill(ESM::Skill::Mercantile).getModified() + repTerm + luckTerm + persTerm) * fatigueTerm;

        int currentDisposition = std::min(100, std::max(0, int(getDerivedDisposition(npc) + currentTemporaryDispositionDelta)));

        float d = 1 - 0.02 * abs(currentDisposition - 50);
        float target1 = d * (playerRating1 - npcRating1 + 50);
        float target2 = d * (playerRating2 - npcRating2 + 50);

        float bribeMod;
        if (type == PT_Bribe10) bribeMod = gmst.find("fBribe10Mod")->getFloat();
        else if (type == PT_Bribe100) bribeMod = gmst.find("fBribe100Mod")->getFloat();
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

                int flee = npcStats.getAiSetting(MWMechanics::CreatureStats::AI_Flee).getBase();
                int fight = npcStats.getAiSetting(MWMechanics::CreatureStats::AI_Fight).getBase();
                npcStats.setAiSetting (MWMechanics::CreatureStats::AI_Flee,
                                       std::max(0, std::min(100, flee + int(std::max(iPerMinChange, s)))));
                // TODO: initiate combat and quit dialogue if fight rating is too high
                // or should setAiSetting handle this?
                npcStats.setAiSetting (MWMechanics::CreatureStats::AI_Fight,
                                       std::max(0, std::min(100, fight + int(std::min(-iPerMinChange, -s)))));
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

            if (success)
            {
                float s = c * fPerDieRollMult * fPerTempMult;
                int flee = npcStats.getAiSetting (CreatureStats::AI_Flee).getBase();
                int fight = npcStats.getAiSetting (CreatureStats::AI_Fight).getBase();
                npcStats.setAiSetting (CreatureStats::AI_Flee,
                                       std::max(0, std::min(100, flee + std::min(-int(iPerMinChange), int(-s)))));
                npcStats.setAiSetting (CreatureStats::AI_Fight,
                                       std::max(0, std::min(100, fight + std::max(int(iPerMinChange), int(s)))));
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

    void MechanicsManager::forceStateUpdate(const MWWorld::Ptr &ptr)
    {
        if(MWWorld::Class::get(ptr).isActor())
            mActors.forceStateUpdate(ptr);
    }

    void MechanicsManager::playAnimationGroup(const MWWorld::Ptr& ptr, const std::string& groupName, int mode, int number)
    {
        if(MWWorld::Class::get(ptr).isActor())
            mActors.playAnimationGroup(ptr, groupName, mode, number);
        else
            mObjects.playAnimationGroup(ptr, groupName, mode, number);
    }
    void MechanicsManager::skipAnimation(const MWWorld::Ptr& ptr)
    {
        if(MWWorld::Class::get(ptr).isActor())
            mActors.skipAnimation(ptr);
        else
            mObjects.skipAnimation(ptr);
    }
    bool MechanicsManager::checkAnimationPlaying(const MWWorld::Ptr& ptr, const std::string &groupName)
    {
        if(MWWorld::Class::get(ptr).isActor())
            return mActors.checkAnimationPlaying(ptr, groupName);
        else
            return false;
    }

    void MechanicsManager::updateMagicEffects(const MWWorld::Ptr &ptr)
    {
        mActors.updateMagicEffects(ptr);
    }

    bool MechanicsManager::toggleAI()
    {
        mAI = !mAI;
        return mAI;
    }

    bool MechanicsManager::isAIActive()
    {
        return mAI;
    }

    void MechanicsManager::playerLoaded()
    {
        mUpdatePlayer = true;
        mClassSelected = true;
        mRaceSelected = true;
        mAI = true;
    }

    bool MechanicsManager::sleepInBed(const MWWorld::Ptr &ptr, const MWWorld::Ptr &bed)
    {
        if(MWBase::Environment::get().getWorld()->getPlayer().isInCombat()) {
            MWBase::Environment::get().getWindowManager()->messageBox("#{sNotifyMessage2}");
            return true;
        }

        MWWorld::Ptr victim;
        if (isAllowedToUse(ptr, bed, victim))
            return false;

        if(commitCrime(ptr, victim, OT_SleepingInOwnedBed))
        {
            MWBase::Environment::get().getWindowManager()->messageBox("#{sNotifyMessage64}");
            return true;
        }
        else
            return false;
    }

    void MechanicsManager::objectOpened(const MWWorld::Ptr &ptr, const MWWorld::Ptr &item)
    {
        MWWorld::Ptr victim;
        if (isAllowedToUse(ptr, item, victim))
            return;
        commitCrime(ptr, victim, OT_Trespassing);
    }

    void MechanicsManager::itemTaken(const MWWorld::Ptr &ptr, const MWWorld::Ptr &item, int count)
    {
        MWWorld::Ptr victim;
        if (isAllowedToUse(ptr, item, victim))
            return;
        commitCrime(ptr, victim, OT_Theft, item.getClass().getValue(item) * count);
    }

    bool MechanicsManager::commitCrime(const MWWorld::Ptr &ptr, const MWWorld::Ptr &victim, OffenseType type, int arg)
    {
        // NOTE: int arg can be from itemTaken() so DON'T modify it, since it is
        //  passed to reportCrime later on in this function.

        // Only player can commit crime
        if (ptr.getRefData().getHandle() != "player")
            return false;

        const MWWorld::ESMStore& esmStore = MWBase::Environment::get().getWorld()->getStore();

        // What amount of alarm did this crime generate?
        int alarm;
        if (type == OT_Trespassing || type == OT_SleepingInOwnedBed)
            alarm = esmStore.get<ESM::GameSetting>().find("iAlarmTresspass")->getInt();
        else if (type == OT_Pickpocket)
            alarm = esmStore.get<ESM::GameSetting>().find("iAlarmPickPocket")->getInt();
        else if (type == OT_Assault)
            alarm = esmStore.get<ESM::GameSetting>().find("iAlarmAttack")->getInt();
        else if (type == OT_Murder)
            alarm = esmStore.get<ESM::GameSetting>().find("iAlarmKilling")->getInt();
        else if (type == OT_Theft)
            alarm = esmStore.get<ESM::GameSetting>().find("iAlarmStealing")->getInt();
        else
            return false;

        // Innocent until proven guilty
        bool reported = false;

        // Find all the NPCs within the alarm radius
        std::vector<MWWorld::Ptr> neighbors;
        mActors.getObjectsInRange(Ogre::Vector3(ptr.getRefData().getPosition().pos),
                                    esmStore.get<ESM::GameSetting>().find("fAlarmRadius")->getInt(), neighbors);

        // Find an actor who witnessed the crime
        for (std::vector<MWWorld::Ptr>::iterator it = neighbors.begin(); it != neighbors.end(); ++it)
        {
            if (   *it == ptr 
                || !it->getClass().isNpc()) continue; // not the player and is an NPC

            // Was the crime seen?
            if ( ( MWBase::Environment::get().getWorld()->getLOS(ptr, *it) && awarenessCheck(ptr, *it) ) ||
                type == OT_Assault )
            {

                // Will the witness report the crime?
                if (it->getClass().getCreatureStats(*it).getAiSetting(CreatureStats::AI_Alarm).getBase() >= alarm)
                {
                    reported = true;
                    int id = MWBase::Environment::get().getWorld()->getPlayer().getNewCrimeId();

                    // Tell everyone, including yourself
                    for (std::vector<MWWorld::Ptr>::iterator it1 = neighbors.begin(); it1 != neighbors.end(); ++it1)
                    {
                        if (   *it == ptr 
                            || !it->getClass().isNpc()) continue; // not the player and is an NPC

                        // TODO: Add more messages
                        if (type == OT_Theft)
                            MWBase::Environment::get().getDialogueManager()->say(*it1, "thief");
                        else if (type == OT_Assault)
                            MWBase::Environment::get().getDialogueManager()->say(*it1, "attack");

                        // Will other witnesses paticipate in crime
                        if (    it1->getClass().getCreatureStats(*it1).getAiSetting(CreatureStats::AI_Alarm).getBase() >= alarm
                            ||  type == OT_Assault )
                        {
                            it1->getClass().getNpcStats(*it1).setCrimeId(id);
                        }

                        // Mark as Alarmed for dialogue
                        it1->getClass().getCreatureStats(*it1).setAlarmed(true);
                    }
                    break; // Someone saw the crime and everyone has been told
                }
            }
        }
        if (reported)
            reportCrime(ptr, victim, type, arg);
        return reported;
    }

    void MechanicsManager::reportCrime(const MWWorld::Ptr &ptr, const MWWorld::Ptr &victim, OffenseType type, int arg)
    {
        const MWWorld::Store<ESM::GameSetting>& store = MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>();

        // Bounty for each type of crime
        if (type == OT_Trespassing || type == OT_SleepingInOwnedBed)
            arg = store.find("iCrimeTresspass")->getInt();
        else if (type == OT_Pickpocket)
            arg = store.find("iCrimePickPocket")->getInt();
        else if (type == OT_Assault)
            arg = store.find("iCrimeAttack")->getInt();
        else if (type == OT_Murder)
            arg = store.find("iCrimeKilling")->getInt();
        else if (type == OT_Theft)
            arg *= store.find("fCrimeStealing")->getFloat();

        MWBase::Environment::get().getWindowManager()->messageBox("#{sCrimeMessage}");
        ptr.getClass().getNpcStats(ptr).setBounty(ptr.getClass().getNpcStats(ptr).getBounty()
                                                  + arg);

        // If committing a crime against a faction member, expell from the faction
        if (!victim.isEmpty() && victim.getClass().isNpc())
        {
            std::string factionID;
            if(!victim.getClass().getNpcStats(victim).getFactionRanks().empty())
                factionID = victim.getClass().getNpcStats(victim).getFactionRanks().begin()->first;
            if (ptr.getClass().getNpcStats(ptr).isSameFaction(victim.getClass().getNpcStats(victim)))
            {
                ptr.getClass().getNpcStats(ptr).expell(factionID);
            }
        }
    }

    bool MechanicsManager::awarenessCheck(const MWWorld::Ptr &ptr, const MWWorld::Ptr &observer)
    {
        if (observer.getClass().getCreatureStats(observer).isDead())
            return false;

        const MWWorld::Store<ESM::GameSetting>& store = MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>();

        CreatureStats& stats = ptr.getClass().getCreatureStats(ptr);

        float invisibility = stats.getMagicEffects().get(ESM::MagicEffect::Invisibility).mMagnitude;
        if (invisibility > 0)
            return false;

        float sneakTerm = 0;
        if (ptr.getClass().getCreatureStats(ptr).getStance(CreatureStats::Stance_Sneak)
                && !MWBase::Environment::get().getWorld()->isSwimming(ptr)
                && MWBase::Environment::get().getWorld()->isOnGround(ptr))
        {
            static float fSneakSkillMult = store.find("fSneakSkillMult")->getFloat();
            static float fSneakBootMult = store.find("fSneakBootMult")->getFloat();
            float sneak = ptr.getClass().getSkill(ptr, ESM::Skill::Sneak);
            int agility = stats.getAttribute(ESM::Attribute::Agility).getModified();
            int luck = stats.getAttribute(ESM::Attribute::Luck).getModified();
            float bootWeight = 0;
            if (ptr.getClass().isNpc())
            {
                MWWorld::InventoryStore& inv = ptr.getClass().getInventoryStore(ptr);
                MWWorld::ContainerStoreIterator it = inv.getSlot(MWWorld::InventoryStore::Slot_Boots);
                if (it != inv.end())
                    bootWeight = it->getClass().getWeight(*it);
            }
            sneakTerm = fSneakSkillMult * sneak + 0.2 * agility + 0.1 * luck + bootWeight * fSneakBootMult;
        }

        static float fSneakDistBase = store.find("fSneakDistanceBase")->getFloat();
        static float fSneakDistMult = store.find("fSneakDistanceMultiplier")->getFloat();

        Ogre::Vector3 pos1 (ptr.getRefData().getPosition().pos);
        Ogre::Vector3 pos2 (observer.getRefData().getPosition().pos);
        float distTerm = fSneakDistBase + fSneakDistMult * pos1.distance(pos2);

        float chameleon = stats.getMagicEffects().get(ESM::MagicEffect::Chameleon).mMagnitude;
        float x = sneakTerm * distTerm * stats.getFatigueTerm() + chameleon + invisibility;

        CreatureStats& observerStats = observer.getClass().getCreatureStats(observer);
        int obsAgility = observerStats.getAttribute(ESM::Attribute::Agility).getModified();
        int obsLuck = observerStats.getAttribute(ESM::Attribute::Luck).getModified();
        float obsBlind = observerStats.getMagicEffects().get(ESM::MagicEffect::Blind).mMagnitude;
        int obsSneak = observer.getClass().getSkill(observer, ESM::Skill::Sneak);

        float obsTerm = obsSneak + 0.2 * obsAgility + 0.1 * obsLuck - obsBlind;

        // is ptr behind the observer?
        static float fSneakNoViewMult = store.find("fSneakNoViewMult")->getFloat();
        static float fSneakViewMult = store.find("fSneakViewMult")->getFloat();
        float y = 0;
        Ogre::Vector3 vec = pos1 - pos2;
        Ogre::Radian angle = observer.getRefData().getBaseNode()->getOrientation().yAxis().angleBetween(vec);
        if (angle < Ogre::Degree(90))
            y = obsTerm * observerStats.getFatigueTerm() * fSneakNoViewMult;
        else
            y = obsTerm * observerStats.getFatigueTerm() * fSneakViewMult;

        float target = x - y;
        int roll = std::rand()/ (static_cast<double> (RAND_MAX) + 1) * 100; // [0, 99]

        return (roll >= target);
    }

    void MechanicsManager::getObjectsInRange(const Ogre::Vector3 &position, float radius, std::vector<MWWorld::Ptr> &objects)
    {
        mActors.getObjectsInRange(position, radius, objects);
        mObjects.getObjectsInRange(position, radius, objects);
    }

    void MechanicsManager::getActorsInRange(const Ogre::Vector3 &position, float radius, std::vector<MWWorld::Ptr> &objects)
    {
        mActors.getObjectsInRange(position, radius, objects);
    }

    std::list<MWWorld::Ptr> MechanicsManager::getActorsFollowing(const MWWorld::Ptr& actor)
    {
        return mActors.getActorsFollowing(actor);
    }

    std::list<MWWorld::Ptr> MechanicsManager::getActorsFighting(const MWWorld::Ptr& actor) {
        return mActors.getActorsFighting(actor);
    }
}

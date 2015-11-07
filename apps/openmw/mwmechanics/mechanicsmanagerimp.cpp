
#include "mechanicsmanagerimp.hpp"
#include "npcstats.hpp"

#include <openengine/misc/rng.hpp>

#include <components/esm/stolenitems.hpp>

#include "../mwworld/esmstore.hpp"
#include "../mwworld/inventorystore.hpp"

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwbase/windowmanager.hpp"
#include "../mwbase/dialoguemanager.hpp"

#include "../mwworld/class.hpp"
#include "../mwworld/player.hpp"

#include "../mwmechanics/aicombat.hpp"
#include "../mwmechanics/aipursue.hpp"

#include <OgreSceneNode.h>

#include "spellcasting.hpp"
#include "autocalcspell.hpp"

#include <limits.h>

namespace
{

    float getFightDistanceBias(const MWWorld::Ptr& actor1, const MWWorld::Ptr& actor2)
    {
        Ogre::Vector3 pos1 (actor1.getRefData().getPosition().pos);
        Ogre::Vector3 pos2 (actor2.getRefData().getPosition().pos);

        float d = pos1.distance(pos2);

        static const int iFightDistanceBase = MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>().find(
                    "iFightDistanceBase")->getInt();
        static const float fFightDistanceMultiplier = MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>().find(
                    "fFightDistanceMultiplier")->getFloat();

        return (iFightDistanceBase - fFightDistanceMultiplier * d);
    }

    float getFightDispositionBias(float disposition)
    {
        static const float fFightDispMult = MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>().find(
                    "fFightDispMult")->getFloat();
        return ((50.f - disposition)  * fFightDispMult);
    }

    void getPersuasionRatings(const MWMechanics::NpcStats& stats, float& rating1, float& rating2, float& rating3, bool player)
    {
        const MWWorld::Store<ESM::GameSetting> &gmst =
            MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>();

        float persTerm = stats.getAttribute(ESM::Attribute::Personality).getModified() / gmst.find("fPersonalityMod")->getFloat();
        float luckTerm = stats.getAttribute(ESM::Attribute::Luck).getModified() / gmst.find("fLuckMod")->getFloat();
        float repTerm = stats.getReputation() * gmst.find("fReputationMod")->getFloat();
        float fatigueTerm = stats.getFatigueTerm();
        float levelTerm = stats.getLevel() * gmst.find("fLevelMod")->getFloat();

        rating1 = (repTerm + luckTerm + persTerm + stats.getSkill(ESM::Skill::Speechcraft).getModified()) * fatigueTerm;

        if (player)
        {
            rating2 = rating1 + levelTerm;
            rating3 = (stats.getSkill(ESM::Skill::Mercantile).getModified() + luckTerm + persTerm) * fatigueTerm;
        }
        else
        {
            rating2 = (levelTerm + repTerm + luckTerm + persTerm + stats.getSkill(ESM::Skill::Speechcraft).getModified()) * fatigueTerm;
            rating3 = (stats.getSkill(ESM::Skill::Mercantile).getModified() + repTerm + luckTerm + persTerm) * fatigueTerm;
        }
    }

}

namespace MWMechanics
{
    void MechanicsManager::buildPlayer()
    {
        MWWorld::Ptr ptr = MWBase::Environment::get().getWorld()->getPlayerPtr();

        MWMechanics::CreatureStats& creatureStats = ptr.getClass().getCreatureStats (ptr);
        MWMechanics::NpcStats& npcStats = ptr.getClass().getNpcStats (ptr);

        const ESM::NPC *player = ptr.get<ESM::NPC>()->mBase;

        // reset
        creatureStats.setLevel(player->mNpdt52.mLevel);
        creatureStats.getSpells().clear();
        creatureStats.modifyMagicEffects(MagicEffects());

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
                }
            }

            const MWWorld::Store<ESM::Skill> &skills =
                esmStore.get<ESM::Skill>();

            MWWorld::Store<ESM::Skill>::iterator iter = skills.begin();
            for (; iter != skills.end(); ++iter)
            {
                if (iter->second.mData.mSpecialization==class_->mData.mSpecialization)
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

        // F_PCStart spells
        static const float fPCbaseMagickaMult = esmStore.get<ESM::GameSetting>().find("fPCbaseMagickaMult")->getFloat();

        float baseMagicka = fPCbaseMagickaMult * creatureStats.getAttribute(ESM::Attribute::Intelligence).getBase();
        bool reachedLimit = false;
        const ESM::Spell* weakestSpell = NULL;
        int minCost = INT_MAX;

        std::vector<std::string> selectedSpells;

        const ESM::Race* race = NULL;
        if (mRaceSelected)
            race = esmStore.get<ESM::Race>().find(player->mRace);

        int skills[ESM::Skill::Length];
        for (int i=0; i<ESM::Skill::Length; ++i)
            skills[i] = npcStats.getSkill(i).getBase();

        int attributes[ESM::Attribute::Length];
        for (int i=0; i<ESM::Attribute::Length; ++i)
            attributes[i] = npcStats.getAttribute(i).getBase();

        const MWWorld::Store<ESM::Spell> &spells =
            esmStore.get<ESM::Spell>();
        for (MWWorld::Store<ESM::Spell>::iterator iter = spells.begin(); iter != spells.end(); ++iter)
        {
            const ESM::Spell* spell = &*iter;

            if (spell->mData.mType != ESM::Spell::ST_Spell)
                continue;
            if (!(spell->mData.mFlags & ESM::Spell::F_PCStart))
                continue;
            if (reachedLimit && spell->mData.mCost <= minCost)
                continue;
            if (race && std::find(race->mPowers.mList.begin(), race->mPowers.mList.end(), spell->mId) != race->mPowers.mList.end())
                continue;
            if (baseMagicka < spell->mData.mCost)
                continue;

            static const float fAutoPCSpellChance = esmStore.get<ESM::GameSetting>().find("fAutoPCSpellChance")->getFloat();
            if (calcAutoCastChance(spell, skills, attributes, -1) < fAutoPCSpellChance)
                continue;

            if (!attrSkillCheck(spell, skills, attributes))
                continue;

            selectedSpells.push_back(spell->mId);

            if (reachedLimit)
            {
                std::vector<std::string>::iterator it = std::find(selectedSpells.begin(), selectedSpells.end(), weakestSpell->mId);
                if (it != selectedSpells.end())
                    selectedSpells.erase(it);

                minCost = INT_MAX;
                for (std::vector<std::string>::iterator weakIt = selectedSpells.begin(); weakIt != selectedSpells.end(); ++weakIt)
                {
                    const ESM::Spell* testSpell = esmStore.get<ESM::Spell>().find(*weakIt);
                    if (testSpell->mData.mCost < minCost)
                    {
                        minCost = testSpell->mData.mCost;
                        weakestSpell = testSpell;
                    }
                }
            }
            else
            {
                if (spell->mData.mCost < minCost)
                {
                    weakestSpell = spell;
                    minCost = weakestSpell->mData.mCost;
                }
                static const unsigned int iAutoPCSpellMax = esmStore.get<ESM::GameSetting>().find("iAutoPCSpellMax")->getInt();
                if (selectedSpells.size() == iAutoPCSpellMax)
                    reachedLimit = true;
            }
        }

        for (std::vector<std::string>::iterator it = selectedSpells.begin(); it != selectedSpells.end(); ++it)
            creatureStats.getSpells().add(*it);

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
    : mWatchedStatsEmpty (true), mUpdatePlayer (true), mClassSelected (false),
      mRaceSelected (false), mAI(true)
    {
        //buildPlayer no longer here, needs to be done explicitely after all subsystems are up and running
    }

    void MechanicsManager::add(const MWWorld::Ptr& ptr)
    {
        if(ptr.getClass().isActor())
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

        if(ptr.getClass().isActor())
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
                if(stats.getAttribute(i) != mWatchedStats.getAttribute(i) || mWatchedStatsEmpty)
                {
                    std::stringstream attrname;
                    attrname << "AttribVal"<<(i+1);

                    mWatchedStats.setAttribute(i, stats.getAttribute(i));
                    winMgr->setValue(attrname.str(), stats.getAttribute(i));
                }
            }

            if(stats.getHealth() != mWatchedStats.getHealth() || mWatchedStatsEmpty)
            {
                static const std::string hbar("HBar");
                mWatchedStats.setHealth(stats.getHealth());
                winMgr->setValue(hbar, stats.getHealth());
            }
            if(stats.getMagicka() != mWatchedStats.getMagicka() || mWatchedStatsEmpty)
            {
                static const std::string mbar("MBar");
                mWatchedStats.setMagicka(stats.getMagicka());
                winMgr->setValue(mbar, stats.getMagicka());
            }
            if(stats.getFatigue() != mWatchedStats.getFatigue() || mWatchedStatsEmpty)
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
                if(stats.getSkill(i) != mWatchedStats.getSkill(i) || mWatchedStatsEmpty)
                {
                    update = true;
                    mWatchedStats.getSkill(i) = stats.getSkill(i);
                    winMgr->setValue((ESM::Skill::SkillEnum)i, stats.getSkill(i));
                }
            }

            if(update)
                winMgr->updateSkillArea();

            winMgr->setValue("level", stats.getLevel());

            mWatchedStatsEmpty = false;

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
            else if (!winMgr->getSelectedSpell().empty())
                winMgr->setSelectedSpell(winMgr->getSelectedSpell(), int(MWMechanics::getSpellSuccessChance(winMgr->getSelectedSpell(), mWatched)));
            else
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
            mActors.addActor(ptr, true);
        }

        mActors.update(duration, paused);
        mObjects.update(duration, paused);
    }

    void MechanicsManager::rest(bool sleep)
    {
        mActors.restoreDynamicStats (sleep);
        mActors.fastForwardAi();
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
        const MWMechanics::NpcStats& npcSkill = ptr.getClass().getNpcStats(ptr);
        float x = static_cast<float>(npcSkill.getBaseDisposition());

        MWWorld::LiveCellRef<ESM::NPC>* npc = ptr.get<ESM::NPC>();
        MWWorld::Ptr playerPtr = MWBase::Environment::get().getWorld()->getPlayerPtr();
        MWWorld::LiveCellRef<ESM::NPC>* player = playerPtr.get<ESM::NPC>();
        const MWMechanics::NpcStats &playerStats = playerPtr.getClass().getNpcStats(playerPtr);

        const MWWorld::Store<ESM::GameSetting>& gmst = MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>();
        static const float fDispRaceMod = gmst.find("fDispRaceMod")->getFloat();
        if (Misc::StringUtils::ciEqual(npc->mBase->mRace, player->mBase->mRace))
            x += fDispRaceMod;

        static const float fDispPersonalityMult = gmst.find("fDispPersonalityMult")->getFloat();
        static const float fDispPersonalityBase = gmst.find("fDispPersonalityBase")->getFloat();
        x += fDispPersonalityMult * (playerStats.getAttribute(ESM::Attribute::Personality).getModified() - fDispPersonalityBase);

        float reaction = 0;
        int rank = 0;
        std::string npcFaction = ptr.getClass().getPrimaryFaction(ptr);

        Misc::StringUtils::toLower(npcFaction);

        if (playerStats.getFactionRanks().find(npcFaction) != playerStats.getFactionRanks().end())
        {
            if (!playerStats.getExpelled(npcFaction))
            {
                // faction reaction towards itself. yes, that exists
                reaction = static_cast<float>(MWBase::Environment::get().getDialogueManager()->getFactionReaction(npcFaction, npcFaction));

                rank = playerStats.getFactionRanks().find(npcFaction)->second;
            }
        }
        else if (!npcFaction.empty())
        {
            std::map<std::string, int>::const_iterator playerFactionIt = playerStats.getFactionRanks().begin();
            for (; playerFactionIt != playerStats.getFactionRanks().end(); ++playerFactionIt)
            {
                std::string itFaction = playerFactionIt->first;

                int itReaction = MWBase::Environment::get().getDialogueManager()->getFactionReaction(npcFaction, itFaction);
                if (playerFactionIt == playerStats.getFactionRanks().begin() || itReaction < reaction)
                    reaction = static_cast<float>(itReaction);
            }
        }
        else
        {
            reaction = 0;
            rank = 0;
        }

        static const float fDispFactionRankMult = gmst.find("fDispFactionRankMult")->getFloat();
        static const float fDispFactionRankBase = gmst.find("fDispFactionRankBase")->getFloat();
        static const float fDispFactionMod = gmst.find("fDispFactionMod")->getFloat();
        x += (fDispFactionRankMult * rank
            + fDispFactionRankBase)
            * fDispFactionMod * reaction;

        static const float fDispCrimeMod = gmst.find("fDispCrimeMod")->getFloat();
        static const float fDispDiseaseMod = gmst.find("fDispDiseaseMod")->getFloat();
        x -= fDispCrimeMod * playerStats.getBounty();
        if (playerStats.hasCommonDisease() || playerStats.hasBlightDisease())
            x += fDispDiseaseMod;

        static const float fDispWeaponDrawn = gmst.find("fDispWeaponDrawn")->getFloat();
        if (playerStats.getDrawState() == MWMechanics::DrawState_Weapon)
            x += fDispWeaponDrawn;

        x += ptr.getClass().getCreatureStats(ptr).getMagicEffects().get(ESM::MagicEffect::Charm).getMagnitude();

        int effective_disposition = std::max(0,std::min(int(x),100));//, normally clamped to [0..100] when used
        return effective_disposition;
    }

    int MechanicsManager::getBarterOffer(const MWWorld::Ptr& ptr,int basePrice, bool buying)
    {
        if (ptr.getTypeName() == typeid(ESM::Creature).name())
            return basePrice;

        const MWMechanics::NpcStats &sellerStats = ptr.getClass().getNpcStats(ptr);

        MWWorld::Ptr playerPtr = MWBase::Environment::get().getWorld()->getPlayerPtr();
        const MWMechanics::NpcStats &playerStats = playerPtr.getClass().getNpcStats(playerPtr);

        // I suppose the temporary disposition change _has_ to be considered here,
        // otherwise one would get different prices when exiting and re-entering the dialogue window...
        int clampedDisposition = std::max(0, std::min(getDerivedDisposition(ptr)
            + MWBase::Environment::get().getDialogueManager()->getTemporaryDispositionChange(),100));
        float a = static_cast<float>(std::min(playerStats.getSkill(ESM::Skill::Mercantile).getModified(), 100));
        float b = std::min(0.1f * playerStats.getAttribute(ESM::Attribute::Luck).getModified(), 10.f);
        float c = std::min(0.2f * playerStats.getAttribute(ESM::Attribute::Personality).getModified(), 10.f);
        float d = static_cast<float>(std::min(sellerStats.getSkill(ESM::Skill::Mercantile).getModified(), 100));
        float e = std::min(0.1f * sellerStats.getAttribute(ESM::Attribute::Luck).getModified(), 10.f);
        float f = std::min(0.2f * sellerStats.getAttribute(ESM::Attribute::Personality).getModified(), 10.f);

        float pcTerm = (clampedDisposition - 50 + a + b + c) * playerStats.getFatigueTerm();
        float npcTerm = (d + e + f) * sellerStats.getFatigueTerm();
        float buyTerm = 0.01f * (100 - 0.5f * (pcTerm - npcTerm));
        float sellTerm = 0.01f * (50 - 0.5f * (npcTerm - pcTerm));

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

        MWMechanics::NpcStats& npcStats = npc.getClass().getNpcStats(npc);

        MWWorld::Ptr playerPtr = MWBase::Environment::get().getWorld()->getPlayerPtr();
        const MWMechanics::NpcStats &playerStats = playerPtr.getClass().getNpcStats(playerPtr);

        float npcRating1, npcRating2, npcRating3;
        getPersuasionRatings(npcStats, npcRating1, npcRating2, npcRating3, false);

        float playerRating1, playerRating2, playerRating3;
        getPersuasionRatings(playerStats, playerRating1, playerRating2, playerRating3, true);

        int currentDisposition = std::min(100, std::max(0, int(getDerivedDisposition(npc) + currentTemporaryDispositionDelta)));

        float d = 1 - 0.02f * abs(currentDisposition - 50);
        float target1 = d * (playerRating1 - npcRating1 + 50);
        float target2 = d * (playerRating2 - npcRating2 + 50);

        float bribeMod;
        if (type == PT_Bribe10) bribeMod = gmst.find("fBribe10Mod")->getFloat();
        else if (type == PT_Bribe100) bribeMod = gmst.find("fBribe100Mod")->getFloat();
        else bribeMod = gmst.find("fBribe1000Mod")->getFloat();

        float target3 = d * (playerRating3 - npcRating3 + 50) + bribeMod;

        float iPerMinChance = floor(gmst.find("iPerMinChance")->getFloat());
        float iPerMinChange = floor(gmst.find("iPerMinChange")->getFloat());
        float fPerDieRollMult = gmst.find("fPerDieRollMult")->getFloat();
        float fPerTempMult = gmst.find("fPerTempMult")->getFloat();

        float x = 0;
        float y = 0;

        int roll = OEngine::Misc::Rng::roll0to99();

        if (type == PT_Admire)
        {
            target1 = std::max(iPerMinChance, target1);
            success = (roll <= target1);
            float c = floor(fPerDieRollMult * (target1 - roll));
            x = success ? std::max(iPerMinChange, c) : c;
        }
        else if (type == PT_Intimidate)
        {
            target2 = std::max(iPerMinChance, target2);

            success =  (roll <= target2);

            float r;
            if (roll != target2)
                r = floor(target2 - roll);
            else
                r = 1;

            if (roll <= target2)
            {
                float s = floor(r * fPerDieRollMult * fPerTempMult);

                int flee = npcStats.getAiSetting(MWMechanics::CreatureStats::AI_Flee).getBase();
                int fight = npcStats.getAiSetting(MWMechanics::CreatureStats::AI_Fight).getBase();
                npcStats.setAiSetting (MWMechanics::CreatureStats::AI_Flee,
                                       std::max(0, std::min(100, flee + int(std::max(iPerMinChange, s)))));
                npcStats.setAiSetting (MWMechanics::CreatureStats::AI_Fight,
                                       std::max(0, std::min(100, fight + int(std::min(-iPerMinChange, -s)))));
            }

            float c = -std::abs(floor(r * fPerDieRollMult));
            if (success)
            {
                if (std::abs(c) < iPerMinChange)
                {
                    x = 0;
                    y = -iPerMinChange;
                }
                else
                {
                    x = -floor(c * fPerTempMult);
                    y = c;
                }
            }
            else
            {
                x = floor(c * fPerTempMult);
                y = c;
            }
        }
        else if (type == PT_Taunt)
        {
            target1 = std::max(iPerMinChance, target1);
            success = (roll <= target1);

            float c = std::abs(floor(target1 - roll));

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
            x = floor(-c * fPerDieRollMult);

            if (success && std::abs(x) < iPerMinChange)
                x = -iPerMinChange;
        }
        else // Bribe
        {
            target3 = std::max(iPerMinChance, target3);
            success = (roll <= target3);
            float c = floor((target3 - roll) * fPerDieRollMult);

            x = success ? std::max(iPerMinChange, c) : c;
        }

        tempChange = type == PT_Intimidate ? x : int(x * fPerTempMult);


        float cappedDispositionChange = tempChange;
        if (currentDisposition + tempChange > 100.f)
            cappedDispositionChange = static_cast<float>(100 - currentDisposition);
        if (currentDisposition + tempChange < 0.f)
            cappedDispositionChange = static_cast<float>(-currentDisposition);

        permChange = floor(cappedDispositionChange / fPerTempMult);
        if (type == PT_Intimidate)
        {
            permChange = success ? -int(cappedDispositionChange/ fPerTempMult) : y;
        }
    }

    void MechanicsManager::forceStateUpdate(const MWWorld::Ptr &ptr)
    {
        if(ptr.getClass().isActor())
            mActors.forceStateUpdate(ptr);
    }

    void MechanicsManager::playAnimationGroup(const MWWorld::Ptr& ptr, const std::string& groupName, int mode, int number)
    {
        if(ptr.getClass().isActor())
            mActors.playAnimationGroup(ptr, groupName, mode, number);
        else
            mObjects.playAnimationGroup(ptr, groupName, mode, number);
    }
    void MechanicsManager::skipAnimation(const MWWorld::Ptr& ptr)
    {
        if(ptr.getClass().isActor())
            mActors.skipAnimation(ptr);
        else
            mObjects.skipAnimation(ptr);
    }
    bool MechanicsManager::checkAnimationPlaying(const MWWorld::Ptr& ptr, const std::string &groupName)
    {
        if(ptr.getClass().isActor())
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

    bool MechanicsManager::isAllowedToUse (const MWWorld::Ptr& ptr, const MWWorld::CellRef& cellref, MWWorld::Ptr& victim)
    {
        const std::string& owner = cellref.getOwner();
        bool isOwned = !owner.empty() && owner != "player";

        const std::string& faction = cellref.getFaction();
        bool isFactionOwned = false;
        if (!faction.empty() && ptr.getClass().isNpc())
        {
            const std::map<std::string, int>& factions = ptr.getClass().getNpcStats(ptr).getFactionRanks();
            std::map<std::string, int>::const_iterator found = factions.find(Misc::StringUtils::lowerCase(faction));
            if (found == factions.end()
                    || found->second < cellref.getFactionRank())
                isFactionOwned = true;
        }

        const std::string& globalVariable = cellref.getGlobalVariable();
        if (!globalVariable.empty() && MWBase::Environment::get().getWorld()->getGlobalInt(Misc::StringUtils::lowerCase(globalVariable)) == 1)
        {
            isOwned = false;
            isFactionOwned = false;
        }

        if (!cellref.getOwner().empty())
            victim = MWBase::Environment::get().getWorld()->searchPtr(cellref.getOwner(), true);

        return (!isOwned && !isFactionOwned);
    }

    bool MechanicsManager::sleepInBed(const MWWorld::Ptr &ptr, const MWWorld::Ptr &bed)
    {
        if (ptr.getClass().getNpcStats(ptr).isWerewolf())
        {
            MWBase::Environment::get().getWindowManager()->messageBox("#{sWerewolfRefusal}");
            return true;
        }

        if(MWBase::Environment::get().getWorld()->getPlayer().isInCombat()) {
            MWBase::Environment::get().getWindowManager()->messageBox("#{sNotifyMessage2}");
            return true;
        }

        MWWorld::Ptr victim;
        if (isAllowedToUse(ptr, bed.getCellRef(), victim))
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
        if (isAllowedToUse(ptr, item.getCellRef(), victim))
            return;
        commitCrime(ptr, victim, OT_Trespassing);
    }

    std::vector<std::pair<std::string, int> > MechanicsManager::getStolenItemOwners(const std::string& itemid)
    {
        std::vector<std::pair<std::string, int> > result;
        StolenItemsMap::const_iterator it = mStolenItems.find(Misc::StringUtils::lowerCase(itemid));
        if (it == mStolenItems.end())
            return result;
        else
        {
            const OwnerMap& owners = it->second;
            for (OwnerMap::const_iterator ownerIt = owners.begin(); ownerIt != owners.end(); ++ownerIt)
                result.push_back(std::make_pair(ownerIt->first.first, ownerIt->second));
            return result;
        }
    }

    bool MechanicsManager::isItemStolenFrom(const std::string &itemid, const std::string &ownerid)
    {
        StolenItemsMap::const_iterator it = mStolenItems.find(Misc::StringUtils::lowerCase(itemid));
        if (it == mStolenItems.end())
            return false;
        const OwnerMap& owners = it->second;
        OwnerMap::const_iterator ownerFound = owners.find(std::make_pair(Misc::StringUtils::lowerCase(ownerid), false));
        return ownerFound != owners.end();
    }

    void MechanicsManager::confiscateStolenItems(const MWWorld::Ptr &player, const MWWorld::Ptr &targetContainer)
    {
        MWWorld::ContainerStore& store = player.getClass().getContainerStore(player);
        for (MWWorld::ContainerStoreIterator it = store.begin(); it != store.end(); ++it)
        {
            StolenItemsMap::iterator stolenIt = mStolenItems.find(Misc::StringUtils::lowerCase(it->getClass().getId(*it)));
            if (stolenIt == mStolenItems.end())
                continue;
            OwnerMap& owners = stolenIt->second;
            int itemCount = it->getRefData().getCount();
            for (OwnerMap::iterator ownerIt = owners.begin(); ownerIt != owners.end();)
            {
                int toRemove = std::min(itemCount, ownerIt->second);
                itemCount -= toRemove;
                ownerIt->second -= toRemove;
                if (ownerIt->second == 0)
                    owners.erase(ownerIt++);
                else
                    ++ownerIt;
            }

            int toMove = it->getRefData().getCount() - itemCount;

            targetContainer.getClass().getContainerStore(targetContainer).add(*it, toMove, targetContainer);
            store.remove(*it, toMove, player);
        }
        // TODO: unhardcode the locklevel
        targetContainer.getClass().lock(targetContainer,50);
    }

    void MechanicsManager::itemTaken(const MWWorld::Ptr &ptr, const MWWorld::Ptr &item, const MWWorld::Ptr& container,
                                     int count)
    {
        if (ptr != MWBase::Environment::get().getWorld()->getPlayerPtr())
            return;

        MWWorld::Ptr victim;

        const MWWorld::CellRef* ownerCellRef = &item.getCellRef();
        if (!container.isEmpty())
        {
            // Inherit the owner of the container
            ownerCellRef = &container.getCellRef();
        }
        else
        {
            if (!item.getCellRef().hasContentFile())
            {
                // this is a manually placed item, which means it was already stolen
                return;
            }
        }

        if (isAllowedToUse(ptr, *ownerCellRef, victim))
            return;

        Owner owner;
        owner.first = ownerCellRef->getOwner();
        owner.second = false;
        if (owner.first.empty())
        {
            owner.first = ownerCellRef->getFaction();
            owner.second = true;
        }
        Misc::StringUtils::toLower(owner.first);

        if (!Misc::StringUtils::ciEqual(item.getCellRef().getRefId(), MWWorld::ContainerStore::sGoldId))
            mStolenItems[Misc::StringUtils::lowerCase(item.getClass().getId(item))][owner] += count;

        commitCrime(ptr, victim, OT_Theft, item.getClass().getValue(item) * count);
    }

    bool MechanicsManager::commitCrime(const MWWorld::Ptr &player, const MWWorld::Ptr &victim, OffenseType type, int arg, bool victimAware)
    {
        // NOTE: victim may be empty

        // Only player can commit crime
        if (player != MWBase::Environment::get().getWorld()->getPlayerPtr())
            return false;

        // Find all the actors within the alarm radius
        std::vector<MWWorld::Ptr> neighbors;

        Ogre::Vector3 from = Ogre::Vector3(player.getRefData().getPosition().pos);
        const MWWorld::ESMStore& esmStore = MWBase::Environment::get().getWorld()->getStore();
        float radius = esmStore.get<ESM::GameSetting>().find("fAlarmRadius")->getFloat();

        mActors.getObjectsInRange(from, radius, neighbors);

        // victim should be considered even beyond alarm radius
        if (!victim.isEmpty() && from.squaredDistance(Ogre::Vector3(victim.getRefData().getPosition().pos)) > radius*radius)
            neighbors.push_back(victim);

        // Did anyone see it?
        bool crimeSeen = false;
        for (std::vector<MWWorld::Ptr>::iterator it = neighbors.begin(); it != neighbors.end(); ++it)
        {
            if (*it == player)
                continue; // skip player
            if (it->getClass().getCreatureStats(*it).isDead())
                continue;

            if ((*it == victim && victimAware)
                    || (MWBase::Environment::get().getWorld()->getLOS(player, *it) && awarenessCheck(player, *it) )
                    // Murder crime can be reported even if no one saw it (hearing is enough, I guess).
                    // TODO: Add mod support for stealth executions!
                    || (type == OT_Murder && *it != victim))
            {
                if (type == OT_Theft || type == OT_Pickpocket)
                    MWBase::Environment::get().getDialogueManager()->say(*it, "thief");
                else if (type == OT_Trespassing)
                    MWBase::Environment::get().getDialogueManager()->say(*it, "intruder");

                // Crime reporting only applies to NPCs
                if (!it->getClass().isNpc())
                    continue;

                if (it->getClass().getCreatureStats(*it).getAiSequence().isInCombat(victim))
                    continue;

                crimeSeen = true;
            }
        }

        if (crimeSeen)
            reportCrime(player, victim, type, arg);
        else if (type == OT_Assault && !victim.isEmpty())
            startCombat(victim, player); // TODO: combat should be started with an "unaware" flag, which makes the victim flee?
        return crimeSeen;
    }

    void MechanicsManager::reportCrime(const MWWorld::Ptr &player, const MWWorld::Ptr &victim, OffenseType type, int arg)
    {
        const MWWorld::Store<ESM::GameSetting>& store = MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>();

        if (type == OT_Murder && !victim.isEmpty())
            victim.getClass().getCreatureStats(victim).notifyMurder();

        // Bounty and disposition penalty for each type of crime
        float disp = 0.f, dispVictim = 0.f;
        if (type == OT_Trespassing || type == OT_SleepingInOwnedBed)
        {
            arg = store.find("iCrimeTresspass")->getInt();
            disp = dispVictim = store.find("iDispTresspass")->getFloat();
        }
        else if (type == OT_Pickpocket)
        {
            arg = store.find("iCrimePickPocket")->getInt();
            disp = dispVictim = store.find("fDispPickPocketMod")->getFloat();
        }
        else if (type == OT_Assault)
        {
            arg = store.find("iCrimeAttack")->getInt();
            disp = store.find("iDispAttackMod")->getFloat();
            dispVictim = store.find("fDispAttacking")->getFloat();
        }
        else if (type == OT_Murder)
        {
            arg = store.find("iCrimeKilling")->getInt();
            disp = dispVictim = store.find("iDispKilling")->getFloat();
        }
        else if (type == OT_Theft)
        {
            disp = dispVictim = store.find("fDispStealing")->getFloat() * arg;
            arg = static_cast<int>(arg * store.find("fCrimeStealing")->getFloat());
            arg = std::max(1, arg); // Minimum bounty of 1, in case items with zero value are stolen
        }

        // Make surrounding actors within alarm distance respond to the crime
        std::vector<MWWorld::Ptr> neighbors;

        const MWWorld::ESMStore& esmStore = MWBase::Environment::get().getWorld()->getStore();

        Ogre::Vector3 from = Ogre::Vector3(player.getRefData().getPosition().pos);
        float radius = esmStore.get<ESM::GameSetting>().find("fAlarmRadius")->getFloat();

        mActors.getObjectsInRange(from, radius, neighbors);

        // victim should be considered even beyond alarm radius
        if (!victim.isEmpty() && from.squaredDistance(Ogre::Vector3(victim.getRefData().getPosition().pos)) > radius*radius)
            neighbors.push_back(victim);

        int id = MWBase::Environment::get().getWorld()->getPlayer().getNewCrimeId();

        // What amount of provocation did this crime generate?
        // Controls whether witnesses will engage combat with the criminal.
        int fight = 0, fightVictim = 0;
        if (type == OT_Trespassing || type == OT_SleepingInOwnedBed)
            fight = fightVictim = esmStore.get<ESM::GameSetting>().find("iFightTrespass")->getInt();
        else if (type == OT_Pickpocket)
        {
            fight = esmStore.get<ESM::GameSetting>().find("iFightPickpocket")->getInt();
            fightVictim = esmStore.get<ESM::GameSetting>().find("iFightPickpocket")->getInt() * 4; // *4 according to research wiki
        }
        else if (type == OT_Assault)
        {
            fight = esmStore.get<ESM::GameSetting>().find("iFightAttacking")->getInt();
            fightVictim = esmStore.get<ESM::GameSetting>().find("iFightAttack")->getInt();
        }
        else if (type == OT_Murder)
            fight = fightVictim = esmStore.get<ESM::GameSetting>().find("iFightKilling")->getInt();
        else if (type == OT_Theft)
            fight = fightVictim = esmStore.get<ESM::GameSetting>().find("fFightStealing")->getInt();

        bool reported = false;

        // Tell everyone (including the original reporter) in alarm range
        for (std::vector<MWWorld::Ptr>::iterator it = neighbors.begin(); it != neighbors.end(); ++it)
        {
            if (   *it == player
                || !it->getClass().isNpc() || it->getClass().getCreatureStats(*it).isDead()) continue;

            if (it->getClass().getCreatureStats(*it).getAiSequence().isInCombat(victim))
                continue;

            // Will the witness report the crime?
            if (it->getClass().getCreatureStats(*it).getAiSetting(CreatureStats::AI_Alarm).getBase() >= 100)
            {
                reported = true;
            }

            if (it->getClass().isClass(*it, "guard"))
            {
                // Mark as Alarmed for dialogue
                it->getClass().getCreatureStats(*it).setAlarmed(true);

                // Set the crime ID, which we will use to calm down participants
                // once the bounty has been paid.
                it->getClass().getNpcStats(*it).setCrimeId(id);

                it->getClass().getCreatureStats(*it).getAiSequence().stack(AiPursue(player), *it);
            }
            else
            {
                float dispTerm = (*it == victim) ? dispVictim : disp;

                float alarmTerm = 0.01f * it->getClass().getCreatureStats(*it).getAiSetting(CreatureStats::AI_Alarm).getBase();
                if (type == OT_Pickpocket && alarmTerm <= 0)
                    alarmTerm = 1.0;

                if (*it != victim)
                    dispTerm *= alarmTerm;

                float fightTerm = static_cast<float>((*it == victim) ? fightVictim : fight);
                fightTerm += getFightDispositionBias(dispTerm);
                fightTerm += getFightDistanceBias(*it, player);
                fightTerm *= alarmTerm;

                int observerFightRating = it->getClass().getCreatureStats(*it).getAiSetting(CreatureStats::AI_Fight).getBase();
                if (observerFightRating + fightTerm > 100)
                    fightTerm = static_cast<float>(100 - observerFightRating);
                fightTerm = std::max(0.f, fightTerm);

                if (observerFightRating + fightTerm >= 100)
                {
                    startCombat(*it, player);

                    NpcStats& observerStats = it->getClass().getNpcStats(*it);
                    // Apply aggression value to the base Fight rating, so that the actor can continue fighting
                    // after a Calm spell wears off
                    observerStats.setAiSetting(CreatureStats::AI_Fight, observerFightRating + static_cast<int>(fightTerm));

                    observerStats.setBaseDisposition(observerStats.getBaseDisposition() + static_cast<int>(dispTerm));

                    // Set the crime ID, which we will use to calm down participants
                    // once the bounty has been paid.
                    observerStats.setCrimeId(id);

                    // Mark as Alarmed for dialogue
                    observerStats.setAlarmed(true);
                }
            }
        }

        if (reported)
        {
            MWBase::Environment::get().getWindowManager()->messageBox("#{sCrimeMessage}");
            player.getClass().getNpcStats(player).setBounty(player.getClass().getNpcStats(player).getBounty()
                                                      + arg);

            // If committing a crime against a faction member, expell from the faction
            if (!victim.isEmpty() && victim.getClass().isNpc())
            {
                std::string factionID = victim.getClass().getPrimaryFaction(victim);

                const std::map<std::string, int>& playerRanks = player.getClass().getNpcStats(player).getFactionRanks();
                if (playerRanks.find(Misc::StringUtils::lowerCase(factionID)) != playerRanks.end())
                {
                    player.getClass().getNpcStats(player).expell(factionID);
                }
            }

            if (type == OT_Assault && !victim.isEmpty()
                    && !victim.getClass().getCreatureStats(victim).getAiSequence().isInCombat(player)
                    && victim.getClass().isNpc())
            {
                // Attacker is in combat with us, but we are not in combat with the attacker yet. Time to fight back.
                // Note: accidental or collateral damage attacks are ignored.
                startCombat(victim, player);

                // Set the crime ID, which we will use to calm down participants
                // once the bounty has been paid.
                victim.getClass().getNpcStats(victim).setCrimeId(id);
            }
        }
    }

    bool MechanicsManager::actorAttacked(const MWWorld::Ptr &ptr, const MWWorld::Ptr &attacker)
    {
        if (ptr == MWBase::Environment::get().getWorld()->getPlayerPtr())
            return false;

        std::list<MWWorld::Ptr> followers = getActorsFollowing(attacker);
        if (std::find(followers.begin(), followers.end(), ptr) != followers.end())
        {
            ptr.getClass().getCreatureStats(ptr).friendlyHit();

            if (ptr.getClass().getCreatureStats(ptr).getFriendlyHits() < 4)
            {
                MWBase::Environment::get().getDialogueManager()->say(ptr, "hit");
                return false;
            }
        }

        // Attacking an NPC that is already in combat with any other NPC is not a crime
        AiSequence& seq = ptr.getClass().getCreatureStats(ptr).getAiSequence();
        bool isFightingNpc = false;
        for (std::list<AiPackage*>::const_iterator it = seq.begin(); it != seq.end(); ++it)
        {
            if ((*it)->getTypeId() == AiPackage::TypeIdCombat)
            {
                MWWorld::Ptr target = static_cast<AiCombat*>(*it)->getTarget();
                if (!target.isEmpty() && target.getClass().isNpc())
                    isFightingNpc = true;
            }
        }

        if (ptr.getClass().isNpc() && !attacker.isEmpty() && !ptr.getClass().getCreatureStats(ptr).getAiSequence().isInCombat(attacker)
                && !isAggressive(ptr, attacker) && !isFightingNpc)
            commitCrime(attacker, ptr, MWBase::MechanicsManager::OT_Assault);

        if (!attacker.isEmpty() && (attacker.getClass().getCreatureStats(attacker).getAiSequence().isInCombat(ptr)
                                    || attacker == MWBase::Environment::get().getWorld()->getPlayerPtr())
                && !ptr.getClass().getCreatureStats(ptr).getAiSequence().isInCombat(attacker))
        {
            // Attacker is in combat with us, but we are not in combat with the attacker yet. Time to fight back.
            // Note: accidental or collateral damage attacks are ignored.
            startCombat(ptr, attacker);
        }

        return true;
    }

    bool MechanicsManager::awarenessCheck(const MWWorld::Ptr &ptr, const MWWorld::Ptr &observer)
    {
        if (observer.getClass().getCreatureStats(observer).isDead() || !observer.getRefData().isEnabled())
            return false;

        const MWWorld::Store<ESM::GameSetting>& store = MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>();

        CreatureStats& stats = ptr.getClass().getCreatureStats(ptr);

        float invisibility = stats.getMagicEffects().get(ESM::MagicEffect::Invisibility).getMagnitude();
        if (invisibility > 0)
            return false;

        float sneakTerm = 0;
        if (ptr.getClass().getCreatureStats(ptr).getStance(CreatureStats::Stance_Sneak)
                && !MWBase::Environment::get().getWorld()->isSwimming(ptr)
                && MWBase::Environment::get().getWorld()->isOnGround(ptr))
        {
            static float fSneakSkillMult = store.find("fSneakSkillMult")->getFloat();
            static float fSneakBootMult = store.find("fSneakBootMult")->getFloat();
            float sneak = static_cast<float>(ptr.getClass().getSkill(ptr, ESM::Skill::Sneak));
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
            sneakTerm = fSneakSkillMult * sneak + 0.2f * agility + 0.1f * luck + bootWeight * fSneakBootMult;
        }

        static float fSneakDistBase = store.find("fSneakDistanceBase")->getFloat();
        static float fSneakDistMult = store.find("fSneakDistanceMultiplier")->getFloat();

        Ogre::Vector3 pos1 (ptr.getRefData().getPosition().pos);
        Ogre::Vector3 pos2 (observer.getRefData().getPosition().pos);
        float distTerm = fSneakDistBase + fSneakDistMult * pos1.distance(pos2);

        float chameleon = stats.getMagicEffects().get(ESM::MagicEffect::Chameleon).getMagnitude();
        float x = sneakTerm * distTerm * stats.getFatigueTerm() + chameleon + invisibility;

        CreatureStats& observerStats = observer.getClass().getCreatureStats(observer);
        int obsAgility = observerStats.getAttribute(ESM::Attribute::Agility).getModified();
        int obsLuck = observerStats.getAttribute(ESM::Attribute::Luck).getModified();
        float obsBlind = observerStats.getMagicEffects().get(ESM::MagicEffect::Blind).getMagnitude();
        int obsSneak = observer.getClass().getSkill(observer, ESM::Skill::Sneak);

        float obsTerm = obsSneak + 0.2f * obsAgility + 0.1f * obsLuck - obsBlind;

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

        return (OEngine::Misc::Rng::roll0to99() >= target);
    }

    void MechanicsManager::startCombat(const MWWorld::Ptr &ptr, const MWWorld::Ptr &target)
    {
        if (ptr.getClass().getCreatureStats(ptr).getAiSequence().isInCombat(target))
            return;
        ptr.getClass().getCreatureStats(ptr).getAiSequence().stack(MWMechanics::AiCombat(target), ptr);
        if (target == MWBase::Environment::get().getWorld()->getPlayerPtr())
        {
            // if guard starts combat with player, guards pursuing player should do the same
            if (ptr.getClass().isClass(ptr, "Guard"))
            {
                for (Actors::PtrActorMap::const_iterator iter = mActors.begin(); iter != mActors.end(); ++iter)
                {
                    if (iter->first.getClass().isClass(iter->first, "Guard"))
                    {
                        MWMechanics::AiSequence& aiSeq = iter->first.getClass().getCreatureStats(iter->first).getAiSequence();
                        if (aiSeq.getTypeId() == MWMechanics::AiPackage::TypeIdPursue)
                        {
                            aiSeq.stopPursuit();
                            aiSeq.stack(MWMechanics::AiCombat(target), ptr);
                        }
                    }
                }
            }
        }

        // Must be done after the target is set up, so that CreatureTargetted dialogue filter works properly
        if (ptr.getClass().isNpc() && !ptr.getClass().getCreatureStats(ptr).isDead())
            MWBase::Environment::get().getDialogueManager()->say(ptr, "attack");
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

    std::list<int> MechanicsManager::getActorsFollowingIndices(const MWWorld::Ptr& actor)
    {
        return mActors.getActorsFollowingIndices(actor);
    }

    std::list<MWWorld::Ptr> MechanicsManager::getActorsFighting(const MWWorld::Ptr& actor) {
        return mActors.getActorsFighting(actor);
    }

    int MechanicsManager::countSavedGameRecords() const
    {
        return 1 // Death counter
                +1; // Stolen items
    }

    void MechanicsManager::write(ESM::ESMWriter &writer, Loading::Listener &listener) const
    {
        mActors.write(writer, listener);

        ESM::StolenItems items;
        items.mStolenItems = mStolenItems;
        writer.startRecord(ESM::REC_STLN);
        items.write(writer);
        writer.endRecord(ESM::REC_STLN);
    }

    void MechanicsManager::readRecord(ESM::ESMReader &reader, uint32_t type)
    {
        if (type == ESM::REC_STLN)
        {
            ESM::StolenItems items;
            items.load(reader);
            mStolenItems = items.mStolenItems;
        }
        else
            mActors.readRecord(reader, type);
    }

    void MechanicsManager::clear()
    {
        mActors.clear();
        mStolenItems.clear();
    }

    bool MechanicsManager::isAggressive(const MWWorld::Ptr &ptr, const MWWorld::Ptr &target)
    {
        int disposition = 50;
        if (ptr.getClass().isNpc())
            disposition = getDerivedDisposition(ptr);

        int fight = std::max(0, ptr.getClass().getCreatureStats(ptr).getAiSetting(CreatureStats::AI_Fight).getModified()
                + static_cast<int>(getFightDistanceBias(ptr, target) + getFightDispositionBias(static_cast<float>(disposition))));

        if (ptr.getClass().isNpc() && target.getClass().isNpc())
        {
            if (target.getClass().getNpcStats(target).isWerewolf() ||
                    (target == MWBase::Environment::get().getWorld()->getPlayerPtr() &&
                     MWBase::Environment::get().getWorld()->getGlobalInt("pcknownwerewolf")))
            {
                const ESM::GameSetting * iWerewolfFightMod = MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>().search("iWerewolfFightMod");
                fight += iWerewolfFightMod->getInt();
            }
        }

        return (fight >= 100);
    }

    void MechanicsManager::keepPlayerAlive()
    {
        MWWorld::Ptr player = MWBase::Environment::get().getWorld()->getPlayerPtr();
        CreatureStats& stats = player.getClass().getCreatureStats(player);
        if (stats.isDead())
        {
            MWMechanics::DynamicStat<float> stat (stats.getHealth());

            if (stat.getModified()<1)
            {
                stat.setModified(1, 0);
                stats.setHealth(stat);
            }
            stats.resurrect();
        }
    }

    bool MechanicsManager::isReadyToBlock(const MWWorld::Ptr &ptr) const
    {
        return mActors.isReadyToBlock(ptr);
    }
}

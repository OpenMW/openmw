#include "mechanicsmanagerimp.hpp"

#include <components/misc/rng.hpp>

#include <components/esm/esmwriter.hpp>
#include <components/esm/stolenitems.hpp>

#include <components/sceneutil/positionattitudetransform.hpp>

#include "../mwworld/esmstore.hpp"
#include "../mwworld/inventorystore.hpp"
#include "../mwworld/class.hpp"
#include "../mwworld/player.hpp"
#include "../mwworld/ptr.hpp"

#include "../mwbase/environment.hpp"
#include "../mwbase/statemanager.hpp"
#include "../mwbase/world.hpp"
#include "../mwbase/windowmanager.hpp"
#include "../mwbase/dialoguemanager.hpp"

#include "aicombat.hpp"
#include "aipursue.hpp"
#include "spellcasting.hpp"
#include "autocalcspell.hpp"
#include "npcstats.hpp"
#include "actorutil.hpp"
#include "combat.hpp"

namespace
{

    float getFightDispositionBias(float disposition)
    {
        static const float fFightDispMult = MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>().find(
                    "fFightDispMult")->mValue.getFloat();
        return ((50.f - disposition)  * fFightDispMult);
    }

    void getPersuasionRatings(const MWMechanics::NpcStats& stats, float& rating1, float& rating2, float& rating3, bool player)
    {
        const MWWorld::Store<ESM::GameSetting> &gmst =
            MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>();

        float persTerm = stats.getAttribute(ESM::Attribute::Personality).getModified() / gmst.find("fPersonalityMod")->mValue.getFloat();
        float luckTerm = stats.getAttribute(ESM::Attribute::Luck).getModified() / gmst.find("fLuckMod")->mValue.getFloat();
        float repTerm = stats.getReputation() * gmst.find("fReputationMod")->mValue.getFloat();
        float fatigueTerm = stats.getFatigueTerm();
        float levelTerm = stats.getLevel() * gmst.find("fLevelMod")->mValue.getFloat();

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
        MWWorld::Ptr ptr = getPlayer();

        MWMechanics::CreatureStats& creatureStats = ptr.getClass().getCreatureStats (ptr);
        MWMechanics::NpcStats& npcStats = ptr.getClass().getNpcStats (ptr);

        npcStats.setNeedRecalcDynamicStats(true);

        const ESM::NPC *player = ptr.get<ESM::NPC>()->mBase;

        // reset
        creatureStats.setLevel(player->mNpdt.mLevel);
        creatureStats.getSpells().clear();
        creatureStats.modifyMagicEffects(MagicEffects());

        for (int i=0; i<27; ++i)
            npcStats.getSkill (i).setBase (player->mNpdt.mSkills[i]);

        creatureStats.setAttribute(ESM::Attribute::Strength, player->mNpdt.mStrength);
        creatureStats.setAttribute(ESM::Attribute::Intelligence, player->mNpdt.mIntelligence);
        creatureStats.setAttribute(ESM::Attribute::Willpower, player->mNpdt.mWillpower);
        creatureStats.setAttribute(ESM::Attribute::Agility, player->mNpdt.mAgility);
        creatureStats.setAttribute(ESM::Attribute::Speed, player->mNpdt.mSpeed);
        creatureStats.setAttribute(ESM::Attribute::Endurance, player->mNpdt.mEndurance);
        creatureStats.setAttribute(ESM::Attribute::Personality, player->mNpdt.mPersonality);
        creatureStats.setAttribute(ESM::Attribute::Luck, player->mNpdt.mLuck);
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

            for (const std::string &power : race->mPowers.mList)
            {
                creatureStats.getSpells().add(power);
            }
        }

        // birthsign
        const std::string &signId =
            MWBase::Environment::get().getWorld()->getPlayer().getBirthSign();

        if (!signId.empty())
        {
            const ESM::BirthSign *sign =
                esmStore.get<ESM::BirthSign>().find(signId);

            for (const std::string &power : sign->mPowers.mList)
            {
                creatureStats.getSpells().add(power);
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
        const ESM::Race* race = nullptr;
        if (mRaceSelected)
            race = esmStore.get<ESM::Race>().find(player->mRace);

        int skills[ESM::Skill::Length];
        for (int i=0; i<ESM::Skill::Length; ++i)
            skills[i] = npcStats.getSkill(i).getBase();

        int attributes[ESM::Attribute::Length];
        for (int i=0; i<ESM::Attribute::Length; ++i)
            attributes[i] = npcStats.getAttribute(i).getBase();

        std::vector<std::string> selectedSpells = autoCalcPlayerSpells(skills, attributes, race);

        for (const std::string &spell : selectedSpells)
            creatureStats.getSpells().add(spell);

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

    // mWatchedTimeToStartDrowning = -1 for correct drowning state check,
    // if stats.getTimeToStartDrowning() == 0 already on game start
    MechanicsManager::MechanicsManager()
    : mWatchedLevel(-1), mWatchedTimeToStartDrowning(-1), mWatchedStatsEmpty (true), mUpdatePlayer (true), mClassSelected (false),
      mRaceSelected (false), mAI(true)
    {
        //buildPlayer no longer here, needs to be done explicitly after all subsystems are up and running
    }

    void MechanicsManager::add(const MWWorld::Ptr& ptr)
    {
        if(ptr.getClass().isActor())
            mActors.addActor(ptr);
        else
            mObjects.addObject(ptr);
    }

    void MechanicsManager::castSpell(const MWWorld::Ptr& ptr, const std::string spellId, bool manualSpell)
    {
        if(ptr.getClass().isActor())
            mActors.castSpell(ptr, spellId, manualSpell);
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

    void MechanicsManager::update(float duration, bool paused)
    {
        if(!mWatched.isEmpty())
        {
            MWBase::WindowManager *winMgr = MWBase::Environment::get().getWindowManager();
            const MWMechanics::NpcStats &stats = mWatched.getClass().getNpcStats(mWatched);
            for(int i = 0;i < ESM::Attribute::Length;++i)
            {
                if(stats.getAttribute(i) != mWatchedAttributes[i] || mWatchedStatsEmpty)
                {
                    std::stringstream attrname;
                    attrname << "AttribVal"<<(i+1);

                    mWatchedAttributes[i] = stats.getAttribute(i);
                    winMgr->setValue(attrname.str(), stats.getAttribute(i));
                }
            }

            if(stats.getHealth() != mWatchedHealth || mWatchedStatsEmpty)
            {
                static const std::string hbar("HBar");
                mWatchedHealth = stats.getHealth();
                winMgr->setValue(hbar, stats.getHealth());
            }
            if(stats.getMagicka() != mWatchedMagicka || mWatchedStatsEmpty)
            {
                static const std::string mbar("MBar");
                mWatchedMagicka = stats.getMagicka();
                winMgr->setValue(mbar, stats.getMagicka());
            }
            if(stats.getFatigue() != mWatchedFatigue || mWatchedStatsEmpty)
            {
                static const std::string fbar("FBar");
                mWatchedFatigue = stats.getFatigue();
                winMgr->setValue(fbar, stats.getFatigue());
            }

            float timeToDrown = stats.getTimeToStartDrowning();

            if(timeToDrown != mWatchedTimeToStartDrowning)
            {
                static const float fHoldBreathTime = MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>()
                        .find("fHoldBreathTime")->mValue.getFloat();

                mWatchedTimeToStartDrowning = timeToDrown;

                if(timeToDrown >= fHoldBreathTime || timeToDrown == -1.0) // -1.0 is a special value during initialization
                    winMgr->setDrowningBarVisibility(false);
                else
                {
                    winMgr->setDrowningBarVisibility(true);
                    winMgr->setDrowningTimeLeft(stats.getTimeToStartDrowning(), fHoldBreathTime);
                }
            }

            //Loop over ESM::Skill::SkillEnum
            for(int i = 0; i < ESM::Skill::Length; ++i)
            {
                if(stats.getSkill(i) != mWatchedSkills[i] || mWatchedStatsEmpty)
                {
                    mWatchedSkills[i] = stats.getSkill(i);
                    winMgr->setValue((ESM::Skill::SkillEnum)i, stats.getSkill(i));
                }
            }

            if(stats.getLevel() != mWatchedLevel)
            {
                mWatchedLevel = stats.getLevel();
                winMgr->setValue("level", mWatchedLevel);
            }

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
            else
            {
                const std::string& spell = winMgr->getSelectedSpell();
                if (!spell.empty())
                    winMgr->setSelectedSpell(spell, int(MWMechanics::getSpellSuccessChance(spell, mWatched)));
                else
                    winMgr->unsetSelectedSpell();
            }

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
            MWWorld::Ptr ptr = getPlayer();
            mActors.removeActor(ptr);
            mActors.addActor(ptr, true);
        }

        mActors.update(duration, paused);
        mObjects.update(duration, paused);
    }

    void MechanicsManager::processChangedSettings(const Settings::CategorySettingVector &changed)
    {
        for (Settings::CategorySettingVector::const_iterator it = changed.begin(); it != changed.end(); ++it)
        {
            if (it->first == "Game" && it->second == "actors processing range")
            {
                int state = MWBase::Environment::get().getStateManager()->getState();
                if (state != MWBase::StateManager::State_Running)
                    continue;

                mActors.updateProcessingRange();

                // Update mechanics for new processing range immediately
                update(0.f, false);
            }
        }
    }

    void MechanicsManager::notifyDied(const MWWorld::Ptr& actor)
    {
        mActors.notifyDied(actor);
    }

    float MechanicsManager::getActorsProcessingRange() const
    {
        return mActors.getProcessingRange();
    }

    bool MechanicsManager::isActorDetected(const MWWorld::Ptr& actor, const MWWorld::Ptr& observer)
    {
        return mActors.isActorDetected(actor, observer);
    }

    bool MechanicsManager::isAttackPreparing(const MWWorld::Ptr& ptr)
    {
        return mActors.isAttackPreparing(ptr);
    }

    bool MechanicsManager::isRunning(const MWWorld::Ptr& ptr)
    {
        return mActors.isRunning(ptr);
    }

    bool MechanicsManager::isSneaking(const MWWorld::Ptr& ptr)
    {
        CreatureStats& stats = ptr.getClass().getCreatureStats(ptr);
        MWBase::World* world = MWBase::Environment::get().getWorld();
        bool animActive = mActors.isSneaking(ptr);
        bool stanceOn = stats.getStance(MWMechanics::CreatureStats::Stance_Sneak);
        bool inair = !world->isOnGround(ptr) && !world->isSwimming(ptr) && !world->isFlying(ptr);
        return stanceOn && (animActive || inair);
    }

    void MechanicsManager::rest(double hours, bool sleep)
    {
        if (sleep)
            MWBase::Environment::get().getWorld()->rest(hours);

        mActors.rest(hours, sleep);
    }

    void MechanicsManager::restoreDynamicStats(MWWorld::Ptr actor, double hours, bool sleep)
    {
        mActors.restoreDynamicStats(actor, hours, sleep);
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

    int MechanicsManager::getDerivedDisposition(const MWWorld::Ptr& ptr, bool addTemporaryDispositionChange)
    {
        const MWMechanics::NpcStats& npcSkill = ptr.getClass().getNpcStats(ptr);
        float x = static_cast<float>(npcSkill.getBaseDisposition());

        MWWorld::LiveCellRef<ESM::NPC>* npc = ptr.get<ESM::NPC>();
        MWWorld::Ptr playerPtr = getPlayer();
        MWWorld::LiveCellRef<ESM::NPC>* player = playerPtr.get<ESM::NPC>();
        const MWMechanics::NpcStats &playerStats = playerPtr.getClass().getNpcStats(playerPtr);

        const MWWorld::Store<ESM::GameSetting>& gmst = MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>();
        static const float fDispRaceMod = gmst.find("fDispRaceMod")->mValue.getFloat();
        if (Misc::StringUtils::ciEqual(npc->mBase->mRace, player->mBase->mRace))
            x += fDispRaceMod;

        static const float fDispPersonalityMult = gmst.find("fDispPersonalityMult")->mValue.getFloat();
        static const float fDispPersonalityBase = gmst.find("fDispPersonalityBase")->mValue.getFloat();
        x += fDispPersonalityMult * (playerStats.getAttribute(ESM::Attribute::Personality).getModified() - fDispPersonalityBase);

        float reaction = 0;
        int rank = 0;
        std::string npcFaction = ptr.getClass().getPrimaryFaction(ptr);

        Misc::StringUtils::lowerCaseInPlace(npcFaction);

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

                // Ignore the faction, if a player was expelled from it.
                if (playerStats.getExpelled(itFaction))
                    continue;

                int itReaction = MWBase::Environment::get().getDialogueManager()->getFactionReaction(npcFaction, itFaction);
                if (playerFactionIt == playerStats.getFactionRanks().begin() || itReaction < reaction)
                {
                    reaction = static_cast<float>(itReaction);
                    rank = playerFactionIt->second;
                }
            }
        }
        else
        {
            reaction = 0;
            rank = 0;
        }

        static const float fDispFactionRankMult = gmst.find("fDispFactionRankMult")->mValue.getFloat();
        static const float fDispFactionRankBase = gmst.find("fDispFactionRankBase")->mValue.getFloat();
        static const float fDispFactionMod = gmst.find("fDispFactionMod")->mValue.getFloat();
        x += (fDispFactionRankMult * rank
            + fDispFactionRankBase)
            * fDispFactionMod * reaction;

        static const float fDispCrimeMod = gmst.find("fDispCrimeMod")->mValue.getFloat();
        static const float fDispDiseaseMod = gmst.find("fDispDiseaseMod")->mValue.getFloat();
        x -= fDispCrimeMod * playerStats.getBounty();
        if (playerStats.hasCommonDisease() || playerStats.hasBlightDisease())
            x += fDispDiseaseMod;

        static const float fDispWeaponDrawn = gmst.find("fDispWeaponDrawn")->mValue.getFloat();
        if (playerStats.getDrawState() == MWMechanics::DrawState_Weapon)
            x += fDispWeaponDrawn;

        x += ptr.getClass().getCreatureStats(ptr).getMagicEffects().get(ESM::MagicEffect::Charm).getMagnitude();

        if(addTemporaryDispositionChange)
          x += MWBase::Environment::get().getDialogueManager()->getTemporaryDispositionChange();

        int effective_disposition = std::max(0,std::min(int(x),100));//, normally clamped to [0..100] when used
        return effective_disposition;
    }

    int MechanicsManager::getBarterOffer(const MWWorld::Ptr& ptr,int basePrice, bool buying)
    {
        if (ptr.getTypeName() == typeid(ESM::Creature).name())
            return basePrice;

        const MWMechanics::NpcStats &sellerStats = ptr.getClass().getNpcStats(ptr);

        MWWorld::Ptr playerPtr = getPlayer();
        const MWMechanics::NpcStats &playerStats = playerPtr.getClass().getNpcStats(playerPtr);

        // I suppose the temporary disposition change (second param to getDerivedDisposition()) _has_ to be considered here,
        // otherwise one would get different prices when exiting and re-entering the dialogue window...
        int clampedDisposition = getDerivedDisposition(ptr);
        float a = static_cast<float>(std::min(playerPtr.getClass().getSkill(playerPtr, ESM::Skill::Mercantile), 100));
        float b = std::min(0.1f * playerStats.getAttribute(ESM::Attribute::Luck).getModified(), 10.f);
        float c = std::min(0.2f * playerStats.getAttribute(ESM::Attribute::Personality).getModified(), 10.f);
        float d = static_cast<float>(std::min(ptr.getClass().getSkill(ptr, ESM::Skill::Mercantile), 100));
        float e = std::min(0.1f * sellerStats.getAttribute(ESM::Attribute::Luck).getModified(), 10.f);
        float f = std::min(0.2f * sellerStats.getAttribute(ESM::Attribute::Personality).getModified(), 10.f);
        float pcTerm = (clampedDisposition - 50 + a + b + c) * playerStats.getFatigueTerm();
        float npcTerm = (d + e + f) * sellerStats.getFatigueTerm();
        float buyTerm = 0.01f * (100 - 0.5f * (pcTerm - npcTerm));
        float sellTerm = 0.01f * (50 - 0.5f * (npcTerm - pcTerm));
        int offerPrice = int(basePrice * (buying ? buyTerm : sellTerm));
        return std::max(1, offerPrice);
    }

    int MechanicsManager::countDeaths (const std::string& id) const
    {
        return mActors.countDeaths (id);
    }

    void MechanicsManager::getPersuasionDispositionChange (const MWWorld::Ptr& npc, PersuasionType type, bool& success, float& tempChange, float& permChange)
    {
        const MWWorld::Store<ESM::GameSetting> &gmst =
            MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>();

        MWMechanics::NpcStats& npcStats = npc.getClass().getNpcStats(npc);

        MWWorld::Ptr playerPtr = getPlayer();
        const MWMechanics::NpcStats &playerStats = playerPtr.getClass().getNpcStats(playerPtr);

        float npcRating1, npcRating2, npcRating3;
        getPersuasionRatings(npcStats, npcRating1, npcRating2, npcRating3, false);

        float playerRating1, playerRating2, playerRating3;
        getPersuasionRatings(playerStats, playerRating1, playerRating2, playerRating3, true);

        int currentDisposition = getDerivedDisposition(npc);

        float d = 1 - 0.02f * abs(currentDisposition - 50);
        float target1 = d * (playerRating1 - npcRating1 + 50);
        float target2 = d * (playerRating2 - npcRating2 + 50);

        float bribeMod;
        if (type == PT_Bribe10) bribeMod = gmst.find("fBribe10Mod")->mValue.getFloat();
        else if (type == PT_Bribe100) bribeMod = gmst.find("fBribe100Mod")->mValue.getFloat();
        else bribeMod = gmst.find("fBribe1000Mod")->mValue.getFloat();

        float target3 = d * (playerRating3 - npcRating3 + 50) + bribeMod;

        float iPerMinChance = floor(gmst.find("iPerMinChance")->mValue.getFloat());
        float iPerMinChange = floor(gmst.find("iPerMinChange")->mValue.getFloat());
        float fPerDieRollMult = gmst.find("fPerDieRollMult")->mValue.getFloat();
        float fPerTempMult = gmst.find("fPerTempMult")->mValue.getFloat();

        float x = 0;
        float y = 0;

        int roll = Misc::Rng::roll0to99();

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

    bool MechanicsManager::playAnimationGroup(const MWWorld::Ptr& ptr, const std::string& groupName, int mode, int number, bool persist)
    {
        if(ptr.getClass().isActor())
            return mActors.playAnimationGroup(ptr, groupName, mode, number, persist);
        else
            return mObjects.playAnimationGroup(ptr, groupName, mode, number, persist);
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

    bool MechanicsManager::onOpen(const MWWorld::Ptr& ptr)
    {
        if(ptr.getClass().isActor())
            return true;
        else
            return mObjects.onOpen(ptr);
    }

    void MechanicsManager::onClose(const MWWorld::Ptr& ptr)
    {
        if(!ptr.getClass().isActor())
            mObjects.onClose(ptr);
    }

    void MechanicsManager::persistAnimationStates()
    {
        mActors.persistAnimationStates();
        mObjects.persistAnimationStates();
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

    bool MechanicsManager::isBoundItem(const MWWorld::Ptr& item)
    {
        static std::set<std::string> boundItemIDCache;

        // If this is empty then we haven't executed the GMST cache logic yet; or there isn't any sMagicBound* GMST's for some reason
        if (boundItemIDCache.empty())
        {
            // Build a list of known bound item ID's
            const MWWorld::Store<ESM::GameSetting> &gameSettings = MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>();

            for (const ESM::GameSetting &currentSetting : gameSettings)
            {
                std::string currentGMSTID = currentSetting.mId;
                Misc::StringUtils::lowerCaseInPlace(currentGMSTID);

                // Don't bother checking this GMST if it's not a sMagicBound* one.
                const std::string& toFind = "smagicbound";
                if (currentGMSTID.compare(0, toFind.length(), toFind) != 0)
                    continue;

                // All sMagicBound* GMST's should be of type string
                std::string currentGMSTValue = currentSetting.mValue.getString();
                Misc::StringUtils::lowerCaseInPlace(currentGMSTValue);

                boundItemIDCache.insert(currentGMSTValue);
            }
        }

        // Perform bound item check and assign the Flag_Bound bit if it passes
        std::string tempItemID = item.getCellRef().getRefId();
        Misc::StringUtils::lowerCaseInPlace(tempItemID);

        if (boundItemIDCache.count(tempItemID) != 0)
            return true;

        return false;
    }

    bool MechanicsManager::isAllowedToUse (const MWWorld::Ptr& ptr, const MWWorld::Ptr& target, MWWorld::Ptr& victim)
    {
        if (target.isEmpty())
            return true;

        const MWWorld::CellRef& cellref = target.getCellRef();
        // there is no harm to use unlocked doors
        int lockLevel = cellref.getLockLevel();
        if (target.getClass().isDoor() &&
            (lockLevel <= 0 || lockLevel == ESM::UnbreakableLock) &&
            ptr.getCellRef().getTrap().empty())
        {
            return true;
        }

        if (!target.getClass().hasToolTip(target))
            return true;

        // TODO: implement a better check to check if target is owned bed
        if (target.getClass().isActivator() && target.getClass().getScript(target).compare(0, 3, "Bed") != 0)
            return true;

        if (target.getClass().isNpc())
        {
            if (target.getClass().getCreatureStats(target).isDead())
                return true;

            if (target.getClass().getCreatureStats(target).getAiSequence().isInCombat())
                return true;

            // check if a player tries to pickpocket a target NPC
            if (target.getClass().getCreatureStats(target).getKnockedDown() || isSneaking(ptr))
                return false;

            return true;
        }

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
            victim = MWBase::Environment::get().getWorld()->searchPtr(cellref.getOwner(), true, false);

        // A special case for evidence chest - we should not allow to take items even if it is technically permitted
        if (Misc::StringUtils::ciEqual(cellref.getRefId(), "stolen_goods"))
            return false;

        return (!isOwned && !isFactionOwned);
    }

    bool MechanicsManager::sleepInBed(const MWWorld::Ptr &ptr, const MWWorld::Ptr &bed)
    {
        if (ptr.getClass().getNpcStats(ptr).isWerewolf())
        {
            MWBase::Environment::get().getWindowManager()->messageBox("#{sWerewolfRefusal}");
            return true;
        }

        if(MWBase::Environment::get().getWorld()->getPlayer().enemiesNearby()) {
            MWBase::Environment::get().getWindowManager()->messageBox("#{sNotifyMessage2}");
            return true;
        }

        MWWorld::Ptr victim;
        if (isAllowedToUse(ptr, bed, victim))
            return false;

        if(commitCrime(ptr, victim, OT_SleepingInOwnedBed, bed.getCellRef().getFaction()))
        {
            MWBase::Environment::get().getWindowManager()->messageBox("#{sNotifyMessage64}");
            return true;
        }
        else
            return false;
    }

    void MechanicsManager::unlockAttempted(const MWWorld::Ptr &ptr, const MWWorld::Ptr &item)
    {
        MWWorld::Ptr victim;
        if (isAllowedToUse(ptr, item, victim))
            return;
        commitCrime(ptr, victim, OT_Trespassing, item.getCellRef().getFaction());
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

    bool MechanicsManager::isItemStolenFrom(const std::string &itemid, const MWWorld::Ptr& ptr)
    {
        StolenItemsMap::const_iterator it = mStolenItems.find(Misc::StringUtils::lowerCase(itemid));
        if (it == mStolenItems.end())
            return false;

        const OwnerMap& owners = it->second;
        const std::string ownerid = ptr.getCellRef().getRefId();
        OwnerMap::const_iterator ownerFound = owners.find(std::make_pair(Misc::StringUtils::lowerCase(ownerid), false));
        if (ownerFound != owners.end())
            return true;

        const std::string factionid = ptr.getClass().getPrimaryFaction(ptr);
        if (!factionid.empty())
        {
            OwnerMap::const_iterator factionOwnerFound = owners.find(std::make_pair(Misc::StringUtils::lowerCase(factionid), true));
            return factionOwnerFound != owners.end();
        }

        return false;
    }

    void MechanicsManager::confiscateStolenItemToOwner(const MWWorld::Ptr &player, const MWWorld::Ptr &item, const MWWorld::Ptr& victim, int count)
    {
        if (player != getPlayer())
            return;

        const std::string itemId = Misc::StringUtils::lowerCase(item.getCellRef().getRefId());

        StolenItemsMap::iterator stolenIt = mStolenItems.find(itemId);
        if (stolenIt == mStolenItems.end())
            return;

        Owner owner;
        owner.first = victim.getCellRef().getRefId();
        owner.second = false;

        const std::string victimFaction = victim.getClass().getPrimaryFaction(victim);
        if (!victimFaction.empty() && Misc::StringUtils::ciEqual(item.getCellRef().getFaction(), victimFaction)) // Is the item faction-owned?
        {
            owner.first = victimFaction;
            owner.second = true;
        }

        Misc::StringUtils::lowerCaseInPlace(owner.first);

        // decrease count of stolen items
        int toRemove = std::min(count, mStolenItems[itemId][owner]);
        mStolenItems[itemId][owner] -= toRemove;
        if (mStolenItems[itemId][owner] == 0)
        {
            // erase owner from stolen items owners
            OwnerMap& owners = stolenIt->second;
            OwnerMap::iterator ownersIt = owners.find(owner);
            if (ownersIt != owners.end())
                owners.erase(ownersIt);
        }

        MWWorld::ContainerStore& store = player.getClass().getContainerStore(player);

        // move items from player to owner and report about theft
        victim.getClass().getContainerStore(victim).add(item, toRemove, victim);
        store.remove(item, toRemove, player);
        commitCrime(player, victim, OT_Theft, item.getCellRef().getFaction(), item.getClass().getValue(item) * toRemove);
    }

    void MechanicsManager::confiscateStolenItems(const MWWorld::Ptr &player, const MWWorld::Ptr &targetContainer)
    {
        MWWorld::ContainerStore& store = player.getClass().getContainerStore(player);
        for (MWWorld::ContainerStoreIterator it = store.begin(); it != store.end(); ++it)
        {
            StolenItemsMap::iterator stolenIt = mStolenItems.find(Misc::StringUtils::lowerCase(it->getCellRef().getRefId()));
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
        targetContainer.getCellRef().lock(50);
    }

    void MechanicsManager::itemTaken(const MWWorld::Ptr &ptr, const MWWorld::Ptr &item, const MWWorld::Ptr& container,
                                     int count, bool alarm)
    {
        if (ptr != getPlayer())
            return;

        MWWorld::Ptr victim;

        bool isAllowed = true;
        const MWWorld::CellRef* ownerCellRef = &item.getCellRef();
        if (!container.isEmpty())
        {
            // Inherit the owner of the container
            ownerCellRef = &container.getCellRef();
            isAllowed = isAllowedToUse(ptr, container, victim);
        }
        else
        {
            isAllowed = isAllowedToUse(ptr, item, victim);
            if (!item.getCellRef().hasContentFile())
            {
                // this is a manually placed item, which means it was already stolen
                return;
            }
        }

        if (isAllowed)
            return;

        Owner owner;
        owner.second = false;
        if (!container.isEmpty() && container.getClass().isActor())
        {
            // "container" is an actor inventory, so just take actor's ID
            owner.first = ownerCellRef->getRefId();
        }
        else
        {
            owner.first = ownerCellRef->getOwner();
            if (owner.first.empty())
            {
                owner.first = ownerCellRef->getFaction();
                owner.second = true;
            }
        }

        Misc::StringUtils::lowerCaseInPlace(owner.first);

        if (!Misc::StringUtils::ciEqual(item.getCellRef().getRefId(), MWWorld::ContainerStore::sGoldId))
        {
            if (victim.isEmpty() || (victim.getClass().isActor() && !victim.getClass().getCreatureStats(victim).isDead()))
                mStolenItems[Misc::StringUtils::lowerCase(item.getCellRef().getRefId())][owner] += count;
        }
        if (alarm)
            commitCrime(ptr, victim, OT_Theft, ownerCellRef->getFaction(), item.getClass().getValue(item) * count);
    }


    bool MechanicsManager::commitCrime(const MWWorld::Ptr &player, const MWWorld::Ptr &victim, OffenseType type, const std::string& factionId, int arg, bool victimAware)
    {
        // NOTE: victim may be empty

        // Only player can commit crime
        if (player != getPlayer())
            return false;

        // Find all the actors within the alarm radius
        std::vector<MWWorld::Ptr> neighbors;

        osg::Vec3f from (player.getRefData().getPosition().asVec3());
        const MWWorld::ESMStore& esmStore = MWBase::Environment::get().getWorld()->getStore();
        float radius = esmStore.get<ESM::GameSetting>().find("fAlarmRadius")->mValue.getFloat();

        mActors.getObjectsInRange(from, radius, neighbors);

        // victim should be considered even beyond alarm radius
        if (!victim.isEmpty() && (from - victim.getRefData().getPosition().asVec3()).length2() > radius*radius)
            neighbors.push_back(victim);

        // get the player's followers / allies (works recursively) that will not report crimes
        std::set<MWWorld::Ptr> playerFollowers;
        getActorsSidingWith(player, playerFollowers);

        // Did anyone see it?
        bool crimeSeen = false;
        for (const MWWorld::Ptr &neighbor : neighbors)
        {
            if (!canReportCrime(neighbor, victim, playerFollowers))
                continue;

            if ((neighbor == victim && victimAware)
                    // Murder crime can be reported even if no one saw it (hearing is enough, I guess).
                    // TODO: Add mod support for stealth executions!
                    || (type == OT_Murder && neighbor != victim)
                    || (MWBase::Environment::get().getWorld()->getLOS(player, neighbor) && awarenessCheck(player, neighbor)))
            {
                // NPC will complain about theft even if he will do nothing about it
                if (type == OT_Theft || type == OT_Pickpocket)
                    MWBase::Environment::get().getDialogueManager()->say(neighbor, "thief");

                crimeSeen = true;
            }
        }

        if (crimeSeen)
            reportCrime(player, victim, type, factionId, arg);
        else if (type == OT_Assault && !victim.isEmpty())
        {
            bool reported = false;
            if (victim.getClass().isClass(victim, "guard")
                && !victim.getClass().getCreatureStats(victim).getAiSequence().hasPackage(AiPackage::TypeIdPursue))
                reported = reportCrime(player, victim, type, std::string(), arg);

            if (!reported)
                startCombat(victim, player); // TODO: combat should be started with an "unaware" flag, which makes the victim flee?
        }
        return crimeSeen;
    }

    bool MechanicsManager::canReportCrime(const MWWorld::Ptr &actor, const MWWorld::Ptr &victim, std::set<MWWorld::Ptr> &playerFollowers)
    {
        if (actor == getPlayer()
            || !actor.getClass().isNpc() || actor.getClass().getCreatureStats(actor).isDead())
            return false;

        if (actor.getClass().getCreatureStats(actor).getAiSequence().isInCombat(victim))
            return false;

        // Unconsious actor can not report about crime and should not become hostile
        if (actor.getClass().getCreatureStats(actor).getKnockedDown())
            return false;

        // Player's followers should not attack player, or try to arrest him
        if (actor.getClass().getCreatureStats(actor).getAiSequence().hasPackage(AiPackage::TypeIdFollow))
        {
            if (playerFollowers.find(actor) != playerFollowers.end())
                return false;
        }

        return true;
    }

    bool MechanicsManager::reportCrime(const MWWorld::Ptr &player, const MWWorld::Ptr &victim, OffenseType type, const std::string& factionId, int arg)
    {
        const MWWorld::Store<ESM::GameSetting>& store = MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>();

        if (type == OT_Murder && !victim.isEmpty())
            victim.getClass().getCreatureStats(victim).notifyMurder();

        // Bounty and disposition penalty for each type of crime
        float disp = 0.f, dispVictim = 0.f;
        if (type == OT_Trespassing || type == OT_SleepingInOwnedBed)
        {
            arg = store.find("iCrimeTresspass")->mValue.getInteger();
            disp = dispVictim = store.find("iDispTresspass")->mValue.getFloat();
        }
        else if (type == OT_Pickpocket)
        {
            arg = store.find("iCrimePickPocket")->mValue.getInteger();
            disp = dispVictim = store.find("fDispPickPocketMod")->mValue.getFloat();
        }
        else if (type == OT_Assault)
        {
            arg = store.find("iCrimeAttack")->mValue.getInteger();
            disp = store.find("iDispAttackMod")->mValue.getFloat();
            dispVictim = store.find("fDispAttacking")->mValue.getFloat();
        }
        else if (type == OT_Murder)
        {
            arg = store.find("iCrimeKilling")->mValue.getInteger();
            disp = dispVictim = store.find("iDispKilling")->mValue.getFloat();
        }
        else if (type == OT_Theft)
        {
            disp = dispVictim = store.find("fDispStealing")->mValue.getFloat() * arg;
            arg = static_cast<int>(arg * store.find("fCrimeStealing")->mValue.getFloat());
            arg = std::max(1, arg); // Minimum bounty of 1, in case items with zero value are stolen
        }

        // Make surrounding actors within alarm distance respond to the crime
        std::vector<MWWorld::Ptr> neighbors;

        const MWWorld::ESMStore& esmStore = MWBase::Environment::get().getWorld()->getStore();

        osg::Vec3f from (player.getRefData().getPosition().asVec3());
        float radius = esmStore.get<ESM::GameSetting>().find("fAlarmRadius")->mValue.getFloat();

        mActors.getObjectsInRange(from, radius, neighbors);

        // victim should be considered even beyond alarm radius
        if (!victim.isEmpty() && (from - victim.getRefData().getPosition().asVec3()).length2() > radius*radius)
            neighbors.push_back(victim);

        int id = MWBase::Environment::get().getWorld()->getPlayer().getNewCrimeId();

        // What amount of provocation did this crime generate?
        // Controls whether witnesses will engage combat with the criminal.
        int fight = 0, fightVictim = 0;
        if (type == OT_Trespassing || type == OT_SleepingInOwnedBed)
            fight = fightVictim = esmStore.get<ESM::GameSetting>().find("iFightTrespass")->mValue.getInteger();
        else if (type == OT_Pickpocket)
        {
            fight = esmStore.get<ESM::GameSetting>().find("iFightPickpocket")->mValue.getInteger();
            fightVictim = esmStore.get<ESM::GameSetting>().find("iFightPickpocket")->mValue.getInteger() * 4; // *4 according to research wiki
        }
        else if (type == OT_Assault)
        {
            fight = esmStore.get<ESM::GameSetting>().find("iFightAttacking")->mValue.getInteger();
            fightVictim = esmStore.get<ESM::GameSetting>().find("iFightAttack")->mValue.getInteger();
        }
        else if (type == OT_Murder)
            fight = fightVictim = esmStore.get<ESM::GameSetting>().find("iFightKilling")->mValue.getInteger();
        else if (type == OT_Theft)
            fight = fightVictim = esmStore.get<ESM::GameSetting>().find("fFightStealing")->mValue.getInteger();

        bool reported = false;

        std::set<MWWorld::Ptr> playerFollowers;
        getActorsSidingWith(player, playerFollowers);

        // Tell everyone (including the original reporter) in alarm range
        for (const MWWorld::Ptr &actor : neighbors)
        {
            if (!canReportCrime(actor, victim, playerFollowers))
                continue;

            // Will the witness report the crime?
            if (actor.getClass().getCreatureStats(actor).getAiSetting(CreatureStats::AI_Alarm).getBase() >= 100)
            {
                reported = true;

                if (type == OT_Trespassing)
                    MWBase::Environment::get().getDialogueManager()->say(actor, "intruder");
            }
        }

        for (const MWWorld::Ptr &actor : neighbors)
        {
            if (!canReportCrime(actor, victim, playerFollowers))
                continue;

            if (reported && actor.getClass().isClass(actor, "guard"))
            {
                // Mark as Alarmed for dialogue
                actor.getClass().getCreatureStats(actor).setAlarmed(true);

                // Set the crime ID, which we will use to calm down participants
                // once the bounty has been paid.
                actor.getClass().getNpcStats(actor).setCrimeId(id);

                if (!actor.getClass().getCreatureStats(actor).getAiSequence().hasPackage(AiPackage::TypeIdPursue))
                {
                    actor.getClass().getCreatureStats(actor).getAiSequence().stack(AiPursue(player), actor);
                }
            }
            else
            {
                float dispTerm = (actor == victim) ? dispVictim : disp;

                float alarmTerm = 0.01f * actor.getClass().getCreatureStats(actor).getAiSetting(CreatureStats::AI_Alarm).getBase();
                if (type == OT_Pickpocket && alarmTerm <= 0)
                    alarmTerm = 1.0;

                if (actor != victim)
                    dispTerm *= alarmTerm;

                float fightTerm = static_cast<float>((actor == victim) ? fightVictim : fight);
                fightTerm += getFightDispositionBias(dispTerm);
                fightTerm += getFightDistanceBias(actor, player);
                fightTerm *= alarmTerm;

                int observerFightRating = actor.getClass().getCreatureStats(actor).getAiSetting(CreatureStats::AI_Fight).getBase();
                if (observerFightRating + fightTerm > 100)
                    fightTerm = static_cast<float>(100 - observerFightRating);
                fightTerm = std::max(0.f, fightTerm);

                if (observerFightRating + fightTerm >= 100)
                {
                    startCombat(actor, player);

                    NpcStats& observerStats = actor.getClass().getNpcStats(actor);
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
            else if (!factionId.empty())
            {
                const std::map<std::string, int>& playerRanks = player.getClass().getNpcStats(player).getFactionRanks();
                if (playerRanks.find(Misc::StringUtils::lowerCase(factionId)) != playerRanks.end())
                {
                    player.getClass().getNpcStats(player).expell(factionId);
                }
            }

            if (type == OT_Assault && !victim.isEmpty()
                    && !victim.getClass().getCreatureStats(victim).getAiSequence().isInCombat(player)
                    && victim.getClass().isNpc())
            {
                // Attacker is in combat with us, but we are not in combat with the attacker yet. Time to fight back.
                // Note: accidental or collateral damage attacks are ignored.
                if (!victim.getClass().getCreatureStats(victim).getAiSequence().hasPackage(AiPackage::TypeIdPursue))
                    startCombat(victim, player);

                // Set the crime ID, which we will use to calm down participants
                // once the bounty has been paid.
                victim.getClass().getNpcStats(victim).setCrimeId(id);
            }
        }

        return reported;
    }

    bool MechanicsManager::actorAttacked(const MWWorld::Ptr &target, const MWWorld::Ptr &attacker)
    {
        const MWWorld::Ptr& player = getPlayer();
        if (target == player || !attacker.getClass().isActor())
            return false;

        MWMechanics::CreatureStats& statsTarget = target.getClass().getCreatureStats(target);
        if (attacker == player)
        {
            std::set<MWWorld::Ptr> followersAttacker;
            getActorsSidingWith(attacker, followersAttacker);
            if (followersAttacker.find(target) != followersAttacker.end())
            {
                statsTarget.friendlyHit();

                if (statsTarget.getFriendlyHits() < 4)
                {
                    MWBase::Environment::get().getDialogueManager()->say(target, "hit");
                    return false;
                }
            }
        }

        if (canCommitCrimeAgainst(target, attacker))
            commitCrime(attacker, target, MWBase::MechanicsManager::OT_Assault);

        AiSequence& seq = statsTarget.getAiSequence();

        if (!attacker.isEmpty()
            && (attacker.getClass().getCreatureStats(attacker).getAiSequence().isInCombat(target) || attacker == player)
            && !seq.isInCombat(attacker))
        {
            // Attacker is in combat with us, but we are not in combat with the attacker yet. Time to fight back.
            // Note: accidental or collateral damage attacks are ignored.
            if (!target.getClass().getCreatureStats(target).getAiSequence().hasPackage(AiPackage::TypeIdPursue))
            {
                // If an actor has OnPCHitMe declared in his script, his Fight = 0 and the attacker is player,
                // he will attack the player only if we will force him (e.g. via StartCombat console command)
                bool peaceful = false;
                std::string script = target.getClass().getScript(target);
                if (!script.empty() && target.getRefData().getLocals().hasVar(script, "onpchitme") && attacker == player)
                {
                    int fight = std::max(0, target.getClass().getCreatureStats(target).getAiSetting(CreatureStats::AI_Fight).getModified());
                    peaceful = (fight == 0);
                }

                if (!peaceful)
                    startCombat(target, attacker);
            }
        }

        return true;
    }

    bool MechanicsManager::canCommitCrimeAgainst(const MWWorld::Ptr &target, const MWWorld::Ptr &attacker)
    {
        const MWMechanics::AiSequence& seq = target.getClass().getCreatureStats(target).getAiSequence();
        return target.getClass().isNpc() && !attacker.isEmpty() && !seq.isInCombat(attacker)
                && !isAggressive(target, attacker) && !seq.isEngagedWithActor()
                && !target.getClass().getCreatureStats(target).getAiSequence().hasPackage(AiPackage::TypeIdPursue);
    }

    void MechanicsManager::actorKilled(const MWWorld::Ptr &victim, const MWWorld::Ptr &attacker)
    {
        if (attacker.isEmpty() || victim.isEmpty())
            return;

        if (victim == attacker)
            return; // known to happen

        if (!victim.getClass().isNpc())
            return; // TODO: implement animal rights

        const MWMechanics::NpcStats& victimStats = victim.getClass().getNpcStats(victim);
        const MWWorld::Ptr &player = getPlayer();
        bool canCommit = attacker == player && canCommitCrimeAgainst(victim, attacker);

        // For now we report only about crimes of player and player's followers
        if (attacker != player)
        {
            std::set<MWWorld::Ptr> playerFollowers;
            getActorsSidingWith(player, playerFollowers);
            if (playerFollowers.find(attacker) == playerFollowers.end())
                return;
        }

        if (!canCommit && victimStats.getCrimeId() == -1)
            return;

        // Simple check for who attacked first: if the player attacked first, a crimeId should be set
        // Doesn't handle possible edge case where no one reported the assault, but in such a case,
        // for bystanders it is not possible to tell who attacked first, anyway.
        commitCrime(player, victim, MWBase::MechanicsManager::OT_Murder);
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
        if (isSneaking(ptr))
        {
            static float fSneakSkillMult = store.find("fSneakSkillMult")->mValue.getFloat();
            static float fSneakBootMult = store.find("fSneakBootMult")->mValue.getFloat();
            float sneak = static_cast<float>(ptr.getClass().getSkill(ptr, ESM::Skill::Sneak));
            int agility = stats.getAttribute(ESM::Attribute::Agility).getModified();
            int luck = stats.getAttribute(ESM::Attribute::Luck).getModified();
            float bootWeight = 0;
            if (ptr.getClass().isNpc() && MWBase::Environment::get().getWorld()->isOnGround(ptr))
            {
                const MWWorld::InventoryStore& inv = ptr.getClass().getInventoryStore(ptr);
                MWWorld::ConstContainerStoreIterator it = inv.getSlot(MWWorld::InventoryStore::Slot_Boots);
                if (it != inv.end())
                    bootWeight = it->getClass().getWeight(*it);
            }
            sneakTerm = fSneakSkillMult * sneak + 0.2f * agility + 0.1f * luck + bootWeight * fSneakBootMult;
        }

        static float fSneakDistBase = store.find("fSneakDistanceBase")->mValue.getFloat();
        static float fSneakDistMult = store.find("fSneakDistanceMultiplier")->mValue.getFloat();

        osg::Vec3f pos1 (ptr.getRefData().getPosition().asVec3());
        osg::Vec3f pos2 (observer.getRefData().getPosition().asVec3());
        float distTerm = fSneakDistBase + fSneakDistMult * (pos1 - pos2).length();

        float chameleon = stats.getMagicEffects().get(ESM::MagicEffect::Chameleon).getMagnitude();
        float x = sneakTerm * distTerm * stats.getFatigueTerm() + chameleon + invisibility;

        CreatureStats& observerStats = observer.getClass().getCreatureStats(observer);
        int obsAgility = observerStats.getAttribute(ESM::Attribute::Agility).getModified();
        int obsLuck = observerStats.getAttribute(ESM::Attribute::Luck).getModified();
        float obsBlind = observerStats.getMagicEffects().get(ESM::MagicEffect::Blind).getMagnitude();
        int obsSneak = observer.getClass().getSkill(observer, ESM::Skill::Sneak);

        float obsTerm = obsSneak + 0.2f * obsAgility + 0.1f * obsLuck - obsBlind;

        // is ptr behind the observer?
        static float fSneakNoViewMult = store.find("fSneakNoViewMult")->mValue.getFloat();
        static float fSneakViewMult = store.find("fSneakViewMult")->mValue.getFloat();
        float y = 0;
        osg::Vec3f vec = pos1 - pos2;
        if (observer.getRefData().getBaseNode())
        {
            osg::Vec3f observerDir = (observer.getRefData().getBaseNode()->getAttitude() * osg::Vec3f(0,1,0));

            float angleRadians = std::acos(observerDir * vec / (observerDir.length() * vec.length()));
            if (angleRadians > osg::DegreesToRadians(90.f))
                y = obsTerm * observerStats.getFatigueTerm() * fSneakNoViewMult;
            else
                y = obsTerm * observerStats.getFatigueTerm() * fSneakViewMult;
        }

        float target = x - y;

        return (Misc::Rng::roll0to99() >= target);
    }

    void MechanicsManager::startCombat(const MWWorld::Ptr &ptr, const MWWorld::Ptr &target)
    {
        CreatureStats& stats = ptr.getClass().getCreatureStats(ptr);

        // Don't add duplicate packages nor add packages to dead actors.
        if (stats.isDead() || stats.getAiSequence().isInCombat(target))
            return;

        // The target is somehow the same as the actor. Early-out.
        if (ptr == target)
        {
            // We don't care about dialogue filters since the target is invalid.
            // We still want to play the combat taunt.
            MWBase::Environment::get().getDialogueManager()->say(ptr, "attack");
            return;
        }

        stats.getAiSequence().stack(MWMechanics::AiCombat(target), ptr);
        if (target == getPlayer())
        {
            // if guard starts combat with player, guards pursuing player should do the same
            if (ptr.getClass().isClass(ptr, "Guard"))
            {
                stats.setHitAttemptActorId(target.getClass().getCreatureStats(target).getActorId()); // Stops guard from ending combat if player is unreachable
                for (Actors::PtrActorMap::const_iterator iter = mActors.begin(); iter != mActors.end(); ++iter)
                {
                    if (iter->first.getClass().isClass(iter->first, "Guard"))
                    {
                        MWMechanics::AiSequence& aiSeq = iter->first.getClass().getCreatureStats(iter->first).getAiSequence();
                        if (aiSeq.getTypeId() == MWMechanics::AiPackage::TypeIdPursue)
                        {
                            aiSeq.stopPursuit();
                            aiSeq.stack(MWMechanics::AiCombat(target), ptr);
                            iter->first.getClass().getCreatureStats(iter->first).setHitAttemptActorId(target.getClass().getCreatureStats(target).getActorId()); // Stops guard from ending combat if player is unreachable
                        }
                    }
                }
            }
        }

        // Must be done after the target is set up, so that CreatureTargetted dialogue filter works properly
        MWBase::Environment::get().getDialogueManager()->say(ptr, "attack");
    }

    void MechanicsManager::getObjectsInRange(const osg::Vec3f &position, float radius, std::vector<MWWorld::Ptr> &objects)
    {
        mActors.getObjectsInRange(position, radius, objects);
        mObjects.getObjectsInRange(position, radius, objects);
    }

    void MechanicsManager::getActorsInRange(const osg::Vec3f &position, float radius, std::vector<MWWorld::Ptr> &objects)
    {
        mActors.getObjectsInRange(position, radius, objects);
    }

    bool MechanicsManager::isAnyActorInRange(const osg::Vec3f &position, float radius)
    {
        return mActors.isAnyObjectInRange(position, radius);
    }

    std::list<MWWorld::Ptr> MechanicsManager::getActorsSidingWith(const MWWorld::Ptr& actor)
    {
        return mActors.getActorsSidingWith(actor);
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

    std::list<MWWorld::Ptr> MechanicsManager::getEnemiesNearby(const MWWorld::Ptr& actor) {
        return mActors.getEnemiesNearby(actor);
    }

    void MechanicsManager::getActorsFollowing(const MWWorld::Ptr& actor, std::set<MWWorld::Ptr>& out) {
        mActors.getActorsFollowing(actor, out);
    }

    void MechanicsManager::getActorsSidingWith(const MWWorld::Ptr& actor, std::set<MWWorld::Ptr>& out) {
        mActors.getActorsSidingWith(actor, out);
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
        mClassSelected = false;
        mRaceSelected = false;
    }

    bool MechanicsManager::isAggressive(const MWWorld::Ptr &ptr, const MWWorld::Ptr &target)
    {
        // Don't become aggressive if a calm effect is active, since it would cause combat to cycle on/off as
        // combat is activated here and then canceled by the calm effect
        if ((ptr.getClass().isNpc() && ptr.getClass().getCreatureStats(ptr).getMagicEffects().get(ESM::MagicEffect::CalmHumanoid).getMagnitude() > 0)
            || (!ptr.getClass().isNpc() && ptr.getClass().getCreatureStats(ptr).getMagicEffects().get(ESM::MagicEffect::CalmCreature).getMagnitude() > 0))
            return false;

        int disposition = 50;
        if (ptr.getClass().isNpc())
            disposition = getDerivedDisposition(ptr, true);

        int fight = ptr.getClass().getCreatureStats(ptr).getAiSetting(CreatureStats::AI_Fight).getModified()
                + static_cast<int>(getFightDistanceBias(ptr, target) + getFightDispositionBias(static_cast<float>(disposition)));

        if (ptr.getClass().isNpc() && target.getClass().isNpc())
        {
            if (target.getClass().getNpcStats(target).isWerewolf() ||
                    (target == getPlayer() &&
                     MWBase::Environment::get().getWorld()->getGlobalInt("pcknownwerewolf")))
            {
                const ESM::GameSetting * iWerewolfFightMod = MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>().find("iWerewolfFightMod");
                fight += iWerewolfFightMod->mValue.getInteger();
            }
        }

        return (fight >= 100);
    }

    void MechanicsManager::resurrect(const MWWorld::Ptr &ptr)
    {
        CreatureStats& stats = ptr.getClass().getCreatureStats(ptr);
        if (stats.isDead())
        {
            stats.resurrect();
            mActors.resurrect(ptr);
        }
    }

    bool MechanicsManager::isCastingSpell(const MWWorld::Ptr &ptr) const
    {
        return mActors.isCastingSpell(ptr);
    }

    bool MechanicsManager::isReadyToBlock(const MWWorld::Ptr &ptr) const
    {
        return mActors.isReadyToBlock(ptr);
    }

    bool MechanicsManager::isAttackingOrSpell(const MWWorld::Ptr &ptr) const
    {
        return mActors.isAttackingOrSpell(ptr);
    }

    void MechanicsManager::setWerewolf(const MWWorld::Ptr& actor, bool werewolf)
    {
        MWMechanics::NpcStats& npcStats = actor.getClass().getNpcStats(actor);

        // The actor does not have to change state
        if (npcStats.isWerewolf() == werewolf)
            return;

        MWWorld::Player* player = &MWBase::Environment::get().getWorld()->getPlayer();

        if (actor == player->getPlayer())
        {
            if (werewolf)
            {
                player->saveStats();
                player->setWerewolfStats();
            }
            else
                player->restoreStats();
        }

        // Werewolfs can not cast spells, so we need to unset the prepared spell if there is one.
        if (npcStats.getDrawState() == MWMechanics::DrawState_Spell)
            npcStats.setDrawState(MWMechanics::DrawState_Nothing);

        npcStats.setWerewolf(werewolf);

        MWWorld::InventoryStore &inv = actor.getClass().getInventoryStore(actor);

        if(werewolf)
        {
            inv.unequipAll(actor);
            inv.equip(MWWorld::InventoryStore::Slot_Robe, inv.ContainerStore::add("werewolfrobe", 1, actor), actor);
        }
        else
        {
            inv.unequipSlot(MWWorld::InventoryStore::Slot_Robe, actor);
            inv.ContainerStore::remove("werewolfrobe", 1, actor);
        }

        if(actor == player->getPlayer())
        {
            MWBase::Environment::get().getWorld()->reattachPlayerCamera();

            // Update the GUI only when called on the player
            MWBase::WindowManager* windowManager = MWBase::Environment::get().getWindowManager();

            if (werewolf)
            {
                windowManager->forceHide(MWGui::GW_Inventory);
                windowManager->forceHide(MWGui::GW_Magic);
            }
            else
            {
                windowManager->unsetForceHide(MWGui::GW_Inventory);
                windowManager->unsetForceHide(MWGui::GW_Magic);
            }

            windowManager->setWerewolfOverlay(werewolf);

            // Witnesses of the player's transformation will make them a globally known werewolf
            std::vector<MWWorld::Ptr> neighbors;
            const MWWorld::Store<ESM::GameSetting>& gmst = MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>();
            getActorsInRange(actor.getRefData().getPosition().asVec3(), gmst.find("fAlarmRadius")->mValue.getFloat(), neighbors);

            bool detected = false, reported = false;
            for (const MWWorld::Ptr& neighbor : neighbors)
            {
                if (neighbor == actor || !neighbor.getClass().isNpc())
                    continue;

                if (MWBase::Environment::get().getWorld()->getLOS(neighbor, actor) && awarenessCheck(actor, neighbor))
                {
                    detected = true;
                    if (neighbor.getClass().getCreatureStats(neighbor).getAiSetting(MWMechanics::CreatureStats::AI_Alarm).getModified() > 0)
                    {
                        reported = true;
                        break;
                    }
                }
            }

            if (detected)
            {
                windowManager->messageBox("#{sWerewolfAlarmMessage}");
                MWBase::Environment::get().getWorld()->setGlobalInt("pcknownwerewolf", 1);

                if (reported)
                {
                    npcStats.setBounty(npcStats.getBounty()+
                                       gmst.find("iWereWolfBounty")->mValue.getInteger());
                }
            }
        }
    }

    void MechanicsManager::applyWerewolfAcrobatics(const MWWorld::Ptr &actor)
    {
        const MWWorld::Store<ESM::GameSetting>& gmst = MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>();
        MWMechanics::NpcStats &stats = actor.getClass().getNpcStats(actor);

        stats.getSkill(ESM::Skill::Acrobatics).setBase(gmst.find("fWerewolfAcrobatics")->mValue.getInteger());
    }

    void MechanicsManager::cleanupSummonedCreature(const MWWorld::Ptr &caster, int creatureActorId)
    {
        mActors.cleanupSummonedCreature(caster.getClass().getCreatureStats(caster), creatureActorId);
    }

}

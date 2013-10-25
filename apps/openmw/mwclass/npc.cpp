
#include "npc.hpp"

#include <memory>

#include <boost/algorithm/string.hpp>

#include <OgreSceneNode.h>

#include <components/esm/loadmgef.hpp>
#include <components/esm/loadnpc.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwbase/mechanicsmanager.hpp"
#include "../mwbase/windowmanager.hpp"
#include "../mwbase/dialoguemanager.hpp"
#include "../mwbase/soundmanager.hpp"

#include "../mwmechanics/creaturestats.hpp"
#include "../mwmechanics/npcstats.hpp"
#include "../mwmechanics/movement.hpp"

#include "../mwworld/ptr.hpp"
#include "../mwworld/actiontalk.hpp"
#include "../mwworld/actionopen.hpp"
#include "../mwworld/failedaction.hpp"
#include "../mwworld/inventorystore.hpp"
#include "../mwworld/customdata.hpp"
#include "../mwworld/physicssystem.hpp"

#include "../mwrender/actors.hpp"
#include "../mwrender/renderinginterface.hpp"

#include "../mwgui/tooltips.hpp"

namespace
{
    const Ogre::Radian kOgrePi (Ogre::Math::PI);
    const Ogre::Radian kOgrePiOverTwo (Ogre::Math::PI / Ogre::Real(2.0));

    struct CustomData : public MWWorld::CustomData
    {
        MWMechanics::NpcStats mNpcStats;
        MWMechanics::Movement mMovement;
        MWWorld::InventoryStore mInventoryStore;

        virtual MWWorld::CustomData *clone() const;
    };

    MWWorld::CustomData *CustomData::clone() const
    {
        return new CustomData (*this);
    }

    void autoCalculateAttributes (const ESM::NPC* npc, MWMechanics::CreatureStats& creatureStats)
    {
        // race bonus
        const ESM::Race *race =
            MWBase::Environment::get().getWorld()->getStore().get<ESM::Race>().find(npc->mRace);

        bool male = (npc->mFlags & ESM::NPC::Female) == 0;

        int level = creatureStats.getLevel();

        for (int i=0; i<ESM::Attribute::Length; ++i)
        {
            const ESM::Race::MaleFemale& attribute = race->mData.mAttributeValues[i];
            creatureStats.getAttribute(i).setBase (male ? attribute.mMale : attribute.mFemale);
        }

        // class bonus
        const ESM::Class *class_ =
            MWBase::Environment::get().getWorld()->getStore().get<ESM::Class>().find(npc->mClass);

        for (int i=0; i<2; ++i)
        {
            int attribute = class_->mData.mAttribute[i];
            if (attribute>=0 && attribute<8)
            {
                creatureStats.getAttribute(attribute).setBase (
                    creatureStats.getAttribute(attribute).getBase() + 10);
            }
        }

        // skill bonus
        for (int attribute=0; attribute<ESM::Attribute::Length; ++attribute)
        {
            float modifierSum = 0;

            for (int j=0; j<ESM::Skill::Length; ++j)
            {
                const ESM::Skill* skill = MWBase::Environment::get().getWorld()->getStore().get<ESM::Skill>().find(j);

                if (skill->mData.mAttribute != attribute)
                    continue;

                // is this a minor or major skill?
                float add=0.2;
                for (int k=0; k<5; ++k)
                {
                    if (class_->mData.mSkills[k][0] == j)
                        add=0.5;
                }
                for (int k=0; k<5; ++k)
                {
                    if (class_->mData.mSkills[k][1] == j)
                        add=1.0;
                }
                modifierSum += add;
            }
            creatureStats.getAttribute(attribute).setBase ( std::min(creatureStats.getAttribute(attribute).getBase()
                + static_cast<int>((level-1) * modifierSum+0.5), 100) );
        }

        // initial health
        int strength = creatureStats.getAttribute(ESM::Attribute::Strength).getBase();
        int endurance = creatureStats.getAttribute(ESM::Attribute::Endurance).getBase();

        int multiplier = 3;

        if (class_->mData.mSpecialization == ESM::Class::Combat)
            multiplier += 2;
        else if (class_->mData.mSpecialization == ESM::Class::Stealth)
            multiplier += 1;

        if (class_->mData.mAttribute[0] == ESM::Attribute::Endurance
            || class_->mData.mAttribute[1] == ESM::Attribute::Endurance)
            multiplier += 1;

        creatureStats.setHealth(static_cast<int> (0.5 * (strength + endurance)) + multiplier * (creatureStats.getLevel() - 1));
    }
}

namespace MWClass
{
    void Npc::ensureCustomData (const MWWorld::Ptr& ptr) const
    {
        static bool inited = false;
        if(!inited)
        {
            const MWBase::World *world = MWBase::Environment::get().getWorld();
            const MWWorld::Store<ESM::GameSetting> &gmst = world->getStore().get<ESM::GameSetting>();

            fMinWalkSpeed = gmst.find("fMinWalkSpeed");
            fMaxWalkSpeed = gmst.find("fMaxWalkSpeed");
            fEncumberedMoveEffect = gmst.find("fEncumberedMoveEffect");
            fSneakSpeedMultiplier = gmst.find("fSneakSpeedMultiplier");
            fAthleticsRunBonus = gmst.find("fAthleticsRunBonus");
            fBaseRunMultiplier = gmst.find("fBaseRunMultiplier");
            fMinFlySpeed = gmst.find("fMinFlySpeed");
            fMaxFlySpeed = gmst.find("fMaxFlySpeed");
            fSwimRunBase = gmst.find("fSwimRunBase");
            fSwimRunAthleticsMult = gmst.find("fSwimRunAthleticsMult");
            fJumpEncumbranceBase = gmst.find("fJumpEncumbranceBase");
            fJumpEncumbranceMultiplier = gmst.find("fJumpEncumbranceMultiplier");
            fJumpAcrobaticsBase = gmst.find("fJumpAcrobaticsBase");
            fJumpAcroMultiplier = gmst.find("fJumpAcroMultiplier");
            fJumpRunMultiplier = gmst.find("fJumpRunMultiplier");
            fWereWolfRunMult = gmst.find("fWereWolfRunMult");

            inited = true;
        }
        if (!ptr.getRefData().getCustomData())
        {
            std::auto_ptr<CustomData> data(new CustomData);

            MWWorld::LiveCellRef<ESM::NPC> *ref = ptr.get<ESM::NPC>();

            // NPC stats
            if (!ref->mBase->mFaction.empty())
            {
                std::string faction = ref->mBase->mFaction;
                Misc::StringUtils::toLower(faction);
                if(ref->mBase->mNpdt52.mGold != -10)
                {
                    data->mNpcStats.getFactionRanks()[faction] = (int)ref->mBase->mNpdt52.mRank;
                }
                else
                {
                    data->mNpcStats.getFactionRanks()[faction] = (int)ref->mBase->mNpdt12.mRank;
                }
            }

            // creature stats
            if(ref->mBase->mNpdt52.mGold != -10)
            {
                for (int i=0; i<27; ++i)
                    data->mNpcStats.getSkill (i).setBase (ref->mBase->mNpdt52.mSkills[i]);

                data->mNpcStats.getAttribute(0).set (ref->mBase->mNpdt52.mStrength);
                data->mNpcStats.getAttribute(1).set (ref->mBase->mNpdt52.mIntelligence);
                data->mNpcStats.getAttribute(2).set (ref->mBase->mNpdt52.mWillpower);
                data->mNpcStats.getAttribute(3).set (ref->mBase->mNpdt52.mAgility);
                data->mNpcStats.getAttribute(4).set (ref->mBase->mNpdt52.mSpeed);
                data->mNpcStats.getAttribute(5).set (ref->mBase->mNpdt52.mEndurance);
                data->mNpcStats.getAttribute(6).set (ref->mBase->mNpdt52.mPersonality);
                data->mNpcStats.getAttribute(7).set (ref->mBase->mNpdt52.mLuck);
                data->mNpcStats.setHealth (ref->mBase->mNpdt52.mHealth);
                data->mNpcStats.setMagicka (ref->mBase->mNpdt52.mMana);
                data->mNpcStats.setFatigue (ref->mBase->mNpdt52.mFatigue);

                data->mNpcStats.setLevel(ref->mBase->mNpdt52.mLevel);
                data->mNpcStats.setBaseDisposition(ref->mBase->mNpdt52.mDisposition);
                data->mNpcStats.setReputation(ref->mBase->mNpdt52.mReputation);
            }
            else
            {
                for (int i=0; i<3; ++i)
                    data->mNpcStats.setDynamic (i, 10);

                data->mNpcStats.setLevel(ref->mBase->mNpdt12.mLevel);
                data->mNpcStats.setBaseDisposition(ref->mBase->mNpdt12.mDisposition);
                data->mNpcStats.setReputation(ref->mBase->mNpdt12.mReputation);

                autoCalculateAttributes(ref->mBase, data->mNpcStats);
            }

            data->mNpcStats.getAiSequence().fill(ref->mBase->mAiPackage);

            data->mNpcStats.setAiSetting (0, ref->mBase->mAiData.mHello);
            data->mNpcStats.setAiSetting (1, ref->mBase->mAiData.mFight);
            data->mNpcStats.setAiSetting (2, ref->mBase->mAiData.mFlee);
            data->mNpcStats.setAiSetting (3, ref->mBase->mAiData.mAlarm);

            // spells
            for (std::vector<std::string>::const_iterator iter (ref->mBase->mSpells.mList.begin());
                iter!=ref->mBase->mSpells.mList.end(); ++iter)
                data->mNpcStats.getSpells().add (*iter);

            // inventory
            data->mInventoryStore.fill(ref->mBase->mInventory, getId(ptr),
                                       MWBase::Environment::get().getWorld()->getStore());

            // store
            ptr.getRefData().setCustomData (data.release());

            getInventoryStore(ptr).autoEquip(ptr);
        }
    }

    std::string Npc::getId (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM::NPC> *ref =
            ptr.get<ESM::NPC>();

        return ref->mBase->mId;
    }

    void Npc::adjustPosition(const MWWorld::Ptr& ptr) const
    {
        MWBase::Environment::get().getWorld()->adjustPosition(ptr);
    }

    void Npc::insertObjectRendering (const MWWorld::Ptr& ptr, MWRender::RenderingInterface& renderingInterface) const
    {
        renderingInterface.getActors().insertNPC(ptr, getInventoryStore(ptr));
    }

    void Npc::insertObject(const MWWorld::Ptr& ptr, MWWorld::PhysicsSystem& physics) const
    {
        physics.addActor(ptr);
        MWBase::Environment::get().getMechanicsManager()->add(ptr);
    }

    bool Npc::isPersistent(const MWWorld::Ptr &actor) const
    {
        MWWorld::LiveCellRef<ESM::NPC>* ref = actor.get<ESM::NPC>();
        return ref->mBase->mPersistent;
    }

    std::string Npc::getModel(const MWWorld::Ptr &ptr) const
    {
        MWWorld::LiveCellRef<ESM::NPC> *ref =
            ptr.get<ESM::NPC>();
        assert(ref->mBase != NULL);

        //std::string headID = ref->mBase->mHead;

        //int end = headID.find_last_of("head_") - 4;
        //std::string bodyRaceID = headID.substr(0, end);

        std::string model = "meshes\\base_anim.nif";
        const ESM::Race* race = MWBase::Environment::get().getWorld()->getStore().get<ESM::Race>().find(ref->mBase->mRace);
        if(race->mData.mFlags & ESM::Race::Beast)
            model = "meshes\\base_animkna.nif";

        return model;

    }

    std::string Npc::getName (const MWWorld::Ptr& ptr) const
    {
        if(getNpcStats(ptr).isWerewolf())
        {
            const MWBase::World *world = MWBase::Environment::get().getWorld();
            const MWWorld::Store<ESM::GameSetting> &gmst = world->getStore().get<ESM::GameSetting>();

            return gmst.find("sWerewolfPopup")->getString();
        }

        MWWorld::LiveCellRef<ESM::NPC> *ref = ptr.get<ESM::NPC>();
        return ref->mBase->mName;
    }

    MWMechanics::CreatureStats& Npc::getCreatureStats (const MWWorld::Ptr& ptr) const
    {
        ensureCustomData (ptr);

        return dynamic_cast<CustomData&> (*ptr.getRefData().getCustomData()).mNpcStats;
    }

    MWMechanics::NpcStats& Npc::getNpcStats (const MWWorld::Ptr& ptr) const
    {
        ensureCustomData (ptr);

        return dynamic_cast<CustomData&> (*ptr.getRefData().getCustomData()).mNpcStats;
    }


    void Npc::hit(const MWWorld::Ptr& ptr, int type) const
    {
        MWBase::World *world = MWBase::Environment::get().getWorld();
        const MWWorld::Store<ESM::GameSetting> &gmst = world->getStore().get<ESM::GameSetting>();

        // Get the weapon used (if hand-to-hand, weapon = inv.end())
        MWWorld::InventoryStore &inv = getInventoryStore(ptr);
        MWWorld::ContainerStoreIterator weaponslot = inv.getSlot(MWWorld::InventoryStore::Slot_CarriedRight);
        MWWorld::Ptr weapon = ((weaponslot != inv.end()) ? *weaponslot : MWWorld::Ptr());
        if(!weapon.isEmpty() && weapon.getTypeName() != typeid(ESM::Weapon).name())
            weapon = MWWorld::Ptr();

        float dist = 100.0f * (!weapon.isEmpty() ?
                               weapon.get<ESM::Weapon>()->mBase->mData.mReach :
                               gmst.find("fHandToHandReach")->getFloat());
        // TODO: Use second to work out the hit angle and where to spawn the blood effect
        MWWorld::Ptr victim = world->getHitContact(ptr, dist).first;
        if(victim.isEmpty()) // Didn't hit anything
            return;

        const MWWorld::Class &othercls = MWWorld::Class::get(victim);
        if(!othercls.isActor()) // Can't hit non-actors
            return;
        MWMechanics::CreatureStats &otherstats = victim.getClass().getCreatureStats(victim);
        if(otherstats.isDead()) // Can't hit dead actors
            return;

        if(ptr.getRefData().getHandle() == "player")
            MWBase::Environment::get().getWindowManager()->setEnemy(victim);

        int weapskill = ESM::Skill::HandToHand;
        if(!weapon.isEmpty())
            weapskill = get(weapon).getEquipmentSkill(weapon);

        MWMechanics::NpcStats &stats = getNpcStats(ptr);
        const MWMechanics::MagicEffects &mageffects = stats.getMagicEffects();
        float hitchance = stats.getSkill(weapskill).getModified() +
                          (stats.getAttribute(ESM::Attribute::Agility).getModified() / 5.0f) +
                          (stats.getAttribute(ESM::Attribute::Luck).getModified() / 10.0f);
        hitchance *= stats.getFatigueTerm();
        hitchance += mageffects.get(MWMechanics::EffectKey(ESM::MagicEffect::FortifyAttack)).mMagnitude -
                     mageffects.get(MWMechanics::EffectKey(ESM::MagicEffect::Blind)).mMagnitude;
        hitchance -= otherstats.getEvasion();

        if((::rand()/(RAND_MAX+1.0)) > hitchance/100.0f)
        {
            othercls.onHit(victim, 0.0f, false, weapon, ptr, false);
            return;
        }

        bool healthdmg;
        float damage = 0.0f;
        if(!weapon.isEmpty())
        {
            const bool weaphashealth = get(weapon).hasItemHealth(weapon);
            const unsigned char *attack = NULL;
            if(type == MWMechanics::CreatureStats::AT_Chop)
                attack = weapon.get<ESM::Weapon>()->mBase->mData.mChop;
            else if(type == MWMechanics::CreatureStats::AT_Slash)
                attack = weapon.get<ESM::Weapon>()->mBase->mData.mSlash;
            else if(type == MWMechanics::CreatureStats::AT_Thrust)
                attack = weapon.get<ESM::Weapon>()->mBase->mData.mThrust;
            if(attack)
            {
                damage  = attack[0] + ((attack[1]-attack[0])*stats.getAttackStrength());
                damage *= 0.5f + (stats.getAttribute(ESM::Attribute::Luck).getModified() / 100.0f);
                if(weaphashealth)
                {
                    int weapmaxhealth = weapon.get<ESM::Weapon>()->mBase->mData.mHealth;
                    if(weapon.getCellRef().mCharge == -1)
                        weapon.getCellRef().mCharge = weapmaxhealth;
                    damage *= float(weapon.getCellRef().mCharge) / weapmaxhealth;
                }
                if(!othercls.hasDetected(victim, ptr))
                {
                    damage *= gmst.find("fCombatCriticalStrikeMult")->getFloat();
                    MWBase::Environment::get().getWindowManager()->messageBox("#{sTargetCriticalStrike}");
                    MWBase::Environment::get().getSoundManager()->playSound3D(victim, "critical damage", 1.0f, 1.0f);
                }
                
                if (!MWBase::Environment::get().getWorld()->getGodModeState())
                    weapon.getCellRef().mCharge -= std::min(std::max(1,
                        (int)(damage * gmst.find("fWeaponDamageMult")->getFloat())), weapon.getCellRef().mCharge);
            }
            healthdmg = true;
        }
        else
        {
            // Note: MCP contains an option to include Strength in hand-to-hand damage
            // calculations. Some mods recommend using it, so we may want to include am
            // option for it.
            float minstrike = gmst.find("fMinHandToHandMult")->getFloat();
            float maxstrike = gmst.find("fMaxHandToHandMult")->getFloat();
            damage  = stats.getSkill(weapskill).getModified();
            damage *= minstrike + ((maxstrike-minstrike)*stats.getAttackStrength());
            if(!othercls.hasDetected(victim, ptr))
            {
                damage *= gmst.find("fCombatCriticalStrikeMult")->getFloat();
                MWBase::Environment::get().getWindowManager()->messageBox("#{sTargetCriticalStrike}");
                MWBase::Environment::get().getSoundManager()->playSound3D(victim, "critical damage", 1.0f, 1.0f);
            }

            healthdmg = (otherstats.getFatigue().getCurrent() < 1.0f);
            if(stats.isWerewolf())
            {
                healthdmg = true;
                // GLOB instead of GMST because it gets updated during a quest
                const MWWorld::Store<ESM::Global> &glob = world->getStore().get<ESM::Global>();
                damage *= glob.find("WerewolfClawMult")->mValue.getFloat();
            }
            if(healthdmg)
                damage *= gmst.find("fHandtoHandHealthPer")->getFloat();

            MWBase::SoundManager *sndMgr = MWBase::Environment::get().getSoundManager();
            if(stats.isWerewolf())
            {
                const ESM::Sound *sound = world->getStore().get<ESM::Sound>().searchRandom("WolfHit");
                if(sound)
                    sndMgr->playSound3D(victim, sound->mId, 1.0f, 1.0f);
            }
            else
                sndMgr->playSound3D(victim, "Hand To Hand Hit", 1.0f, 1.0f);
        }
        if(ptr.getRefData().getHandle() == "player")
            skillUsageSucceeded(ptr, weapskill, 0);

        othercls.onHit(victim, damage, healthdmg, weapon, ptr, true);
    }

    void Npc::onHit(const MWWorld::Ptr &ptr, float damage, bool ishealth, const MWWorld::Ptr &object, const MWWorld::Ptr &attacker, bool successful) const
    {
        MWBase::SoundManager *sndMgr = MWBase::Environment::get().getSoundManager();

        // NOTE: 'object' and/or 'attacker' may be empty.

        if(!successful)
        {
            // TODO: Handle HitAttemptOnMe script function

            // Missed
            sndMgr->playSound3D(ptr, "miss", 1.0f, 1.0f);
            return;
        }

        if(!object.isEmpty())
            getCreatureStats(ptr).setLastHitObject(get(object).getId(object));

        if(!attacker.isEmpty() && attacker.getRefData().getHandle() == "player")
        {
            const std::string &script = ptr.get<ESM::NPC>()->mBase->mScript;
            /* Set the OnPCHitMe script variable. The script is responsible for clearing it. */
            if(!script.empty())
                ptr.getRefData().getLocals().setVarByInt(script, "onpchitme", 1);
        }

        if(damage > 0.0f)
        {
            // 'ptr' is losing health. Play a 'hit' voiced dialog entry if not already saying
            // something, alert the character controller, scripts, etc.

            MWBase::Environment::get().getDialogueManager()->say(ptr, "hit");

            if(object.isEmpty())
            {
                if(ishealth)
                    damage /= std::min(1.0f + getArmorRating(ptr)/std::max(1.0f, damage), 4.0f);
            }
            else if(ishealth)
            {
                // Hit percentages:
                // cuirass = 30%
                // shield, helmet, greaves, boots, pauldrons = 10% each
                // guantlets = 5% each
                static const int hitslots[20] = {
                    MWWorld::InventoryStore::Slot_Cuirass, MWWorld::InventoryStore::Slot_Cuirass,
                    MWWorld::InventoryStore::Slot_Cuirass, MWWorld::InventoryStore::Slot_Cuirass,
                    MWWorld::InventoryStore::Slot_Cuirass, MWWorld::InventoryStore::Slot_Cuirass,
                    MWWorld::InventoryStore::Slot_CarriedLeft, MWWorld::InventoryStore::Slot_CarriedLeft,
                    MWWorld::InventoryStore::Slot_Helmet, MWWorld::InventoryStore::Slot_Helmet,
                    MWWorld::InventoryStore::Slot_Greaves, MWWorld::InventoryStore::Slot_Greaves,
                    MWWorld::InventoryStore::Slot_Boots, MWWorld::InventoryStore::Slot_Boots,
                    MWWorld::InventoryStore::Slot_LeftPauldron, MWWorld::InventoryStore::Slot_LeftPauldron,
                    MWWorld::InventoryStore::Slot_RightPauldron, MWWorld::InventoryStore::Slot_RightPauldron,
                    MWWorld::InventoryStore::Slot_LeftGauntlet, MWWorld::InventoryStore::Slot_RightGauntlet
                };
                int hitslot = hitslots[(int)(::rand()/(RAND_MAX+1.0)*20.0)];

                float damagediff = damage;
                damage /= std::min(1.0f + getArmorRating(ptr)/std::max(1.0f, damage), 4.0f);
                damagediff -= damage;

                MWWorld::InventoryStore &inv = getInventoryStore(ptr);
                MWWorld::ContainerStoreIterator armorslot = inv.getSlot(hitslot);
                MWWorld::Ptr armor = ((armorslot != inv.end()) ? *armorslot : MWWorld::Ptr());
                if(!armor.isEmpty() && armor.getTypeName() == typeid(ESM::Armor).name())
                {
                    ESM::CellRef &armorref = armor.getCellRef();
                    if(armorref.mCharge == -1)
                        armorref.mCharge = armor.get<ESM::Armor>()->mBase->mData.mHealth;
                    armorref.mCharge -= std::min(std::max(1, (int)damagediff),
                                                 armorref.mCharge);
                    switch(get(armor).getEquipmentSkill(armor))
                    {
                        case ESM::Skill::LightArmor:
                            sndMgr->playSound3D(ptr, "Light Armor Hit", 1.0f, 1.0f);
                            break;
                        case ESM::Skill::MediumArmor:
                            sndMgr->playSound3D(ptr, "Medium Armor Hit", 1.0f, 1.0f);
                            break;
                        case ESM::Skill::HeavyArmor:
                            sndMgr->playSound3D(ptr, "Heavy Armor Hit", 1.0f, 1.0f);
                            break;
                    }
                }
            }
        }

        if(ishealth)
        {
            if(damage > 0.0f)
                sndMgr->playSound3D(ptr, "Health Damage", 1.0f, 1.0f);
            float health = getCreatureStats(ptr).getHealth().getCurrent() - damage;
            setActorHealth(ptr, health, attacker);
        }
        else
        {
            MWMechanics::DynamicStat<float> fatigue(getCreatureStats(ptr).getFatigue());
            fatigue.setCurrent(fatigue.getCurrent() - damage);
            getCreatureStats(ptr).setFatigue(fatigue);
        }
    }

    void Npc::setActorHealth(const MWWorld::Ptr& ptr, float health, const MWWorld::Ptr& attacker) const
    {
        MWMechanics::CreatureStats &crstats = getCreatureStats(ptr);
        bool wasDead = crstats.isDead();

        MWMechanics::DynamicStat<float> stat(crstats.getHealth());
        stat.setCurrent(health);
        crstats.setHealth(stat);

        if(!wasDead && crstats.isDead())
        {
            // actor was just killed
        }
        else if(wasDead && !crstats.isDead())
        {
            // actor was just resurrected
        }
    }


    boost::shared_ptr<MWWorld::Action> Npc::activate (const MWWorld::Ptr& ptr,
        const MWWorld::Ptr& actor) const
    {
        if(get(actor).isNpc() && get(actor).getNpcStats(actor).isWerewolf())
        {
            const MWWorld::ESMStore &store = MWBase::Environment::get().getWorld()->getStore();
            const ESM::Sound *sound = store.get<ESM::Sound>().searchRandom("WolfNPC");

            boost::shared_ptr<MWWorld::Action> action(new MWWorld::FailedAction("#{sWerewolfRefusal}"));
            if(sound) action->setSound(sound->mId);

            return action;
        }
        if(getCreatureStats(ptr).isDead())
            return boost::shared_ptr<MWWorld::Action>(new MWWorld::ActionOpen(ptr, true));
        if(get(actor).getStance(actor, MWWorld::Class::Sneak))
            return boost::shared_ptr<MWWorld::Action>(new MWWorld::ActionOpen(ptr)); // stealing
        return boost::shared_ptr<MWWorld::Action>(new MWWorld::ActionTalk(ptr));
    }

    MWWorld::ContainerStore& Npc::getContainerStore (const MWWorld::Ptr& ptr)
        const
    {
        ensureCustomData (ptr);

        return dynamic_cast<CustomData&> (*ptr.getRefData().getCustomData()).mInventoryStore;
    }

    MWWorld::InventoryStore& Npc::getInventoryStore (const MWWorld::Ptr& ptr)
        const
    {
        ensureCustomData (ptr);

        return dynamic_cast<CustomData&> (*ptr.getRefData().getCustomData()).mInventoryStore;
    }

    std::string Npc::getScript (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM::NPC> *ref =
            ptr.get<ESM::NPC>();

        return ref->mBase->mScript;
    }

    void Npc::setForceStance (const MWWorld::Ptr& ptr, Stance stance, bool force) const
    {
        MWMechanics::NpcStats& stats = getNpcStats (ptr);

        switch (stance)
        {
            case Run:

                stats.setMovementFlag (MWMechanics::NpcStats::Flag_ForceRun, force);
                break;

            case Sneak:

                stats.setMovementFlag (MWMechanics::NpcStats::Flag_ForceSneak, force);
                break;

            case Combat:

                throw std::runtime_error ("combat stance not enforcable for NPCs");
        }
    }

    void Npc::setStance (const MWWorld::Ptr& ptr, Stance stance, bool set) const
    {
        MWMechanics::NpcStats& stats = getNpcStats (ptr);

        switch (stance)
        {
            case Run:

                stats.setMovementFlag (MWMechanics::NpcStats::Flag_Run, set);
                break;

            case Sneak:

                stats.setMovementFlag (MWMechanics::NpcStats::Flag_Sneak, set);
                break;

            case Combat:

                // Combat stance ignored for now; need to be determined based on draw state instead of
                // being maunally set.
                break;
        }
    }

    bool Npc::getStance (const MWWorld::Ptr& ptr, Stance stance, bool ignoreForce) const
    {
        MWMechanics::NpcStats& stats = getNpcStats (ptr);

        switch (stance)
        {
            case Run:

                if (!ignoreForce && stats.getMovementFlag (MWMechanics::NpcStats::Flag_ForceRun))
                    return true;

                return stats.getMovementFlag (MWMechanics::NpcStats::Flag_Run);

            case Sneak:

                if (!ignoreForce && stats.getMovementFlag (MWMechanics::NpcStats::Flag_ForceSneak))
                    return true;

                return stats.getMovementFlag (MWMechanics::NpcStats::Flag_Sneak);

            case Combat:

                return false;
        }

        return false;
    }

    float Npc::getSpeed(const MWWorld::Ptr& ptr) const
    {
        const MWBase::World *world = MWBase::Environment::get().getWorld();
        const CustomData *npcdata = static_cast<const CustomData*>(ptr.getRefData().getCustomData());
        const MWMechanics::MagicEffects &mageffects = npcdata->mNpcStats.getMagicEffects();

        const float normalizedEncumbrance = Npc::getEncumbrance(ptr) / Npc::getCapacity(ptr);

        float walkSpeed = fMinWalkSpeed->getFloat() + 0.01f*npcdata->mNpcStats.getAttribute(ESM::Attribute::Speed).getModified()*
                                                      (fMaxWalkSpeed->getFloat() - fMinWalkSpeed->getFloat());
        walkSpeed *= 1.0f - fEncumberedMoveEffect->getFloat()*normalizedEncumbrance;
        walkSpeed = std::max(0.0f, walkSpeed);
        if(Npc::getStance(ptr, Sneak, false))
            walkSpeed *= fSneakSpeedMultiplier->getFloat();

        float runSpeed = walkSpeed*(0.01f * npcdata->mNpcStats.getSkill(ESM::Skill::Athletics).getModified() *
                                    fAthleticsRunBonus->getFloat() + fBaseRunMultiplier->getFloat());
        if(npcdata->mNpcStats.isWerewolf())
            runSpeed *= fWereWolfRunMult->getFloat();

        float moveSpeed;
        if(normalizedEncumbrance >= 1.0f)
            moveSpeed = 0.0f;
        else if(mageffects.get(MWMechanics::EffectKey(10/*levitate*/)).mMagnitude > 0)
        {
            float flySpeed = 0.01f*(npcdata->mNpcStats.getAttribute(ESM::Attribute::Speed).getModified() +
                                    mageffects.get(MWMechanics::EffectKey(10/*levitate*/)).mMagnitude);
            flySpeed = fMinFlySpeed->getFloat() + flySpeed*(fMaxFlySpeed->getFloat() - fMinFlySpeed->getFloat());
            flySpeed *= 1.0f - fEncumberedMoveEffect->getFloat() * normalizedEncumbrance;
            flySpeed = std::max(0.0f, flySpeed);
            moveSpeed = flySpeed;
        }
        else if(world->isSwimming(ptr))
        {
            float swimSpeed = walkSpeed;
            if(Npc::getStance(ptr, Run, false))
                swimSpeed = runSpeed;
            swimSpeed *= 1.0f + 0.01f * mageffects.get(MWMechanics::EffectKey(1/*swift swim*/)).mMagnitude;
            swimSpeed *= fSwimRunBase->getFloat() + 0.01f*npcdata->mNpcStats.getSkill(ESM::Skill::Athletics).getModified()*
                                                    fSwimRunAthleticsMult->getFloat();
            moveSpeed = swimSpeed;
        }
        else if(Npc::getStance(ptr, Run, false) && !Npc::getStance(ptr, Sneak, false))
            moveSpeed = runSpeed;
        else
            moveSpeed = walkSpeed;
        if(getMovementSettings(ptr).mPosition[0] != 0 && getMovementSettings(ptr).mPosition[1] == 0)
            moveSpeed *= 0.75f;

        return moveSpeed;
    }

    float Npc::getJump(const MWWorld::Ptr &ptr) const
    {
        const CustomData *npcdata = static_cast<const CustomData*>(ptr.getRefData().getCustomData());
        const MWMechanics::MagicEffects &mageffects = npcdata->mNpcStats.getMagicEffects();
        const float encumbranceTerm = fJumpEncumbranceBase->getFloat() +
                                          fJumpEncumbranceMultiplier->getFloat() *
                                          (1.0f - Npc::getEncumbrance(ptr)/Npc::getCapacity(ptr));

        float a = npcdata->mNpcStats.getSkill(ESM::Skill::Acrobatics).getModified();
        float b = 0.0f;
        if(a > 50.0f)
        {
            b = a - 50.0f;
            a = 50.0f;
        }

        float x = fJumpAcrobaticsBase->getFloat() +
                  std::pow(a / 15.0f, fJumpAcroMultiplier->getFloat());
        x += 3.0f * b * fJumpAcroMultiplier->getFloat();
        x += mageffects.get(MWMechanics::EffectKey(ESM::MagicEffect::Jump)).mMagnitude * 64;
        x *= encumbranceTerm;

        if(Npc::getStance(ptr, Run, false))
            x *= fJumpRunMultiplier->getFloat();
        x *= npcdata->mNpcStats.getFatigueTerm();
        x -= -627.2f;/*gravity constant*/
        x /= 3.0f;

        return x;
    }

    float Npc::getFallDamage(const MWWorld::Ptr &ptr, float fallHeight) const
    {
        MWBase::World *world = MWBase::Environment::get().getWorld();
        const MWWorld::Store<ESM::GameSetting> &gmst = world->getStore().get<ESM::GameSetting>();

        const float fallDistanceMin = gmst.find("fFallDamageDistanceMin")->getFloat();

        if (fallHeight >= fallDistanceMin)
        {
            const float acrobaticsSkill = MWWorld::Class::get(ptr).getNpcStats (ptr).getSkill(ESM::Skill::Acrobatics).getModified();
            const CustomData *npcdata = static_cast<const CustomData*>(ptr.getRefData().getCustomData());
            const float jumpSpellBonus = npcdata->mNpcStats.getMagicEffects().get(MWMechanics::EffectKey(ESM::MagicEffect::Jump)).mMagnitude;
            const float fallAcroBase = gmst.find("fFallAcroBase")->getFloat();
            const float fallAcroMult = gmst.find("fFallAcroMult")->getFloat();
            const float fallDistanceBase = gmst.find("fFallDistanceBase")->getFloat();
            const float fallDistanceMult = gmst.find("fFallDistanceMult")->getFloat();

            float x = fallHeight - fallDistanceMin;
            x -= (1.5 * acrobaticsSkill) + jumpSpellBonus;
            x = std::max(0.0f, x);

            float a = fallAcroBase + fallAcroMult * (100 - acrobaticsSkill);
            x = fallDistanceBase + fallDistanceMult * x;
            x *= a;

            return x;
        }

        return 0;
    }

    MWMechanics::Movement& Npc::getMovementSettings (const MWWorld::Ptr& ptr) const
    {
        ensureCustomData (ptr);

        return dynamic_cast<CustomData&> (*ptr.getRefData().getCustomData()).mMovement;
    }

    Ogre::Vector3 Npc::getMovementVector (const MWWorld::Ptr& ptr) const
    {
        MWMechanics::Movement &movement = getMovementSettings(ptr);
        Ogre::Vector3 vec(movement.mPosition);
        movement.mPosition[0] = 0.0f;
        movement.mPosition[1] = 0.0f;
        movement.mPosition[2] = 0.0f;
        return vec;
    }

    Ogre::Vector3 Npc::getRotationVector (const MWWorld::Ptr& ptr) const
    {
        MWMechanics::Movement &movement = getMovementSettings(ptr);
        Ogre::Vector3 vec(movement.mRotation);
        movement.mRotation[0] = 0.0f;
        movement.mRotation[1] = 0.0f;
        movement.mRotation[2] = 0.0f;
        return vec;
    }

    bool Npc::isEssential (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM::NPC> *ref =
            ptr.get<ESM::NPC>();

        return ref->mBase->mFlags & ESM::NPC::Essential;
    }
    
    void Npc::registerSelf()
    {
        boost::shared_ptr<Class> instance (new Npc);
        registerClass (typeid (ESM::NPC).name(), instance);
    }

    bool Npc::hasToolTip (const MWWorld::Ptr& ptr) const
    {
        /// \todo We don't want tooltips for NPCs in combat mode.

        return true;
    }

    MWGui::ToolTipInfo Npc::getToolTipInfo (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM::NPC> *ref = ptr.get<ESM::NPC>();

        bool fullHelp = MWBase::Environment::get().getWindowManager()->getFullHelp();
        MWGui::ToolTipInfo info;

        info.caption = getName(ptr);
        if(fullHelp && getNpcStats(ptr).isWerewolf())
        {
            info.caption += " (";
            info.caption += ref->mBase->mName;
            info.caption += ")";
        }

        if(fullHelp)
            info.text = MWGui::ToolTips::getMiscString(ref->mBase->mScript, "Script");

        return info;
    }

    float Npc::getCapacity (const MWWorld::Ptr& ptr) const
    {
        const MWMechanics::CreatureStats& stats = getCreatureStats (ptr);
        return stats.getAttribute(0).getModified()*5;
    }

    float Npc::getEncumbrance (const MWWorld::Ptr& ptr) const
    {
        const MWMechanics::NpcStats &stats = getNpcStats(ptr);

        // According to UESP, inventory weight is ignored in werewolf form. Does that include
        // feather and burden effects?
        float weight = 0.0f;
        if(!stats.isWerewolf())
        {
            weight  = getContainerStore(ptr).getWeight();
            weight -= stats.getMagicEffects().get(MWMechanics::EffectKey(ESM::MagicEffect::Feather)).mMagnitude;
            weight += stats.getMagicEffects().get(MWMechanics::EffectKey(ESM::MagicEffect::Burden)).mMagnitude;
            if(weight < 0.0f)
                weight = 0.0f;
        }

        return weight;
    }

    bool Npc::apply (const MWWorld::Ptr& ptr, const std::string& id,
        const MWWorld::Ptr& actor) const
    {
        MWMechanics::CreatureStats& stats = getCreatureStats (ptr);

        /// \todo consider instant effects

        return stats.getActiveSpells().addSpell (id, actor);
    }

    void Npc::skillUsageSucceeded (const MWWorld::Ptr& ptr, int skill, int usageType) const
    {
        MWMechanics::NpcStats& stats = getNpcStats (ptr);

        MWWorld::LiveCellRef<ESM::NPC> *ref = ptr.get<ESM::NPC>();

        const ESM::Class *class_ =
            MWBase::Environment::get().getWorld()->getStore().get<ESM::Class>().find (
                ref->mBase->mClass
            );

        stats.useSkill (skill, *class_, usageType);
    }

    float Npc::getArmorRating (const MWWorld::Ptr& ptr) const
    {
        const MWBase::World *world = MWBase::Environment::get().getWorld();
        const MWWorld::Store<ESM::GameSetting> &gmst = world->getStore().get<ESM::GameSetting>();

        MWMechanics::NpcStats &stats = getNpcStats(ptr);
        MWWorld::InventoryStore &invStore = getInventoryStore(ptr);

        int iBaseArmorSkill = gmst.find("iBaseArmorSkill")->getInt();
        float fUnarmoredBase1 = gmst.find("fUnarmoredBase1")->getFloat();
        float fUnarmoredBase2 = gmst.find("fUnarmoredBase2")->getFloat();
        int unarmoredSkill = stats.getSkill(ESM::Skill::Unarmored).getModified();

        int ratings[MWWorld::InventoryStore::Slots];
        for(int i = 0;i < MWWorld::InventoryStore::Slots;i++)
        {
            MWWorld::ContainerStoreIterator it = invStore.getSlot(i);
            if (it == invStore.end() || it->getTypeName() != typeid(ESM::Armor).name())
            {
                // unarmored
                ratings[i] = (fUnarmoredBase1 * unarmoredSkill) * (fUnarmoredBase2 * unarmoredSkill);
            }
            else
            {
                MWWorld::LiveCellRef<ESM::Armor> *ref = it->get<ESM::Armor>();

                int armorSkillType = MWWorld::Class::get(*it).getEquipmentSkill(*it);
                int armorSkill = stats.getSkill(armorSkillType).getModified();

                if(ref->mBase->mData.mWeight == 0)
                    ratings[i] = ref->mBase->mData.mArmor;
                else
                    ratings[i] = ref->mBase->mData.mArmor * armorSkill / iBaseArmorSkill;
            }
        }

        float shield = stats.getMagicEffects().get(ESM::MagicEffect::Shield).mMagnitude;

        return ratings[MWWorld::InventoryStore::Slot_Cuirass] * 0.3f
                + (ratings[MWWorld::InventoryStore::Slot_CarriedLeft] + ratings[MWWorld::InventoryStore::Slot_Helmet]
                    + ratings[MWWorld::InventoryStore::Slot_Greaves] + ratings[MWWorld::InventoryStore::Slot_Boots]
                    + ratings[MWWorld::InventoryStore::Slot_LeftPauldron] + ratings[MWWorld::InventoryStore::Slot_RightPauldron]
                    ) * 0.1f
                + (ratings[MWWorld::InventoryStore::Slot_LeftGauntlet] + ratings[MWWorld::InventoryStore::Slot_RightGauntlet])
                    * 0.05f
                + shield;
    }


    void Npc::adjustRotation(const MWWorld::Ptr& ptr,float& x,float& y,float& z) const
    {
        y = 0;
        x = 0;
    }

    void Npc::adjustScale(const MWWorld::Ptr &ptr, float &scale) const
    {
        MWWorld::LiveCellRef<ESM::NPC> *ref =
            ptr.get<ESM::NPC>();

        const ESM::Race* race =
                MWBase::Environment::get().getWorld()->getStore().get<ESM::Race>().find(ref->mBase->mRace);

        if (ref->mBase->isMale())
            scale *= race->mData.mHeight.mMale;
        else
            scale *= race->mData.mHeight.mFemale;
    }

    int Npc::getServices(const MWWorld::Ptr &actor) const
    {
        MWWorld::LiveCellRef<ESM::NPC>* ref = actor.get<ESM::NPC>();
        if (ref->mBase->mHasAI)
            return ref->mBase->mAiData.mServices;
        else
            return 0;
    }


    std::string Npc::getSoundIdFromSndGen(const MWWorld::Ptr &ptr, const std::string &name) const
    {
        if(name == "left")
        {
            MWBase::World *world = MWBase::Environment::get().getWorld();
            Ogre::Vector3 pos(ptr.getRefData().getPosition().pos);
            if(world->isSwimming(ptr))
                return "Swim Left";
            if(world->isUnderwater(ptr.getCell(), pos))
                return "FootWaterLeft";
            if(world->isOnGround(ptr))
            {
                MWWorld::InventoryStore &inv = Npc::getInventoryStore(ptr);
                MWWorld::ContainerStoreIterator boots = inv.getSlot(MWWorld::InventoryStore::Slot_Boots);
                if(boots == inv.end() || boots->getTypeName() != typeid(ESM::Armor).name())
                    return "FootBareLeft";

                switch(Class::get(*boots).getEquipmentSkill(*boots))
                {
                    case ESM::Skill::LightArmor:
                        return "FootLightLeft";
                    case ESM::Skill::MediumArmor:
                        return "FootMedLeft";
                    case ESM::Skill::HeavyArmor:
                        return "FootHeavyLeft";
                }
            }
            return "";
        }
        if(name == "right")
        {
            MWBase::World *world = MWBase::Environment::get().getWorld();
            Ogre::Vector3 pos(ptr.getRefData().getPosition().pos);
            if(world->isSwimming(ptr))
                return "Swim Right";
            if(world->isUnderwater(ptr.getCell(), pos))
                return "FootWaterRight";
            if(world->isOnGround(ptr))
            {
                MWWorld::InventoryStore &inv = Npc::getInventoryStore(ptr);
                MWWorld::ContainerStoreIterator boots = inv.getSlot(MWWorld::InventoryStore::Slot_Boots);
                if(boots == inv.end() || boots->getTypeName() != typeid(ESM::Armor).name())
                    return "FootBareRight";

                switch(Class::get(*boots).getEquipmentSkill(*boots))
                {
                    case ESM::Skill::LightArmor:
                        return "FootLightRight";
                    case ESM::Skill::MediumArmor:
                        return "FootMedRight";
                    case ESM::Skill::HeavyArmor:
                        return "FootHeavyRight";
                }
            }
            return "";
        }
        if(name == "land")
        {
            MWBase::World *world = MWBase::Environment::get().getWorld();
            Ogre::Vector3 pos(ptr.getRefData().getPosition().pos);
            if(world->isUnderwater(ptr.getCell(), pos))
                return "DefaultLandWater";
            if(world->isOnGround(ptr))
                return "Body Fall Medium";
            return "";
        }
        if(name == "swimleft")
            return "Swim Left";
        if(name == "swimright")
            return "Swim Right";
        // TODO: I have no idea what these are supposed to do for NPCs since they use
        // voiced dialog for various conditions like health loss and combat taunts. Maybe
        // only for biped creatures?
        if(name == "moan")
            return "";
        if(name == "roar")
            return "";
        if(name == "scream")
            return "";

        throw std::runtime_error(std::string("Unexpected soundgen type: ")+name);
    }

    MWWorld::Ptr
    Npc::copyToCellImpl(const MWWorld::Ptr &ptr, MWWorld::CellStore &cell) const
    {
        MWWorld::LiveCellRef<ESM::NPC> *ref =
            ptr.get<ESM::NPC>();

        return MWWorld::Ptr(&cell.mNpcs.insert(*ref), &cell);
    }

    const ESM::GameSetting *Npc::fMinWalkSpeed;
    const ESM::GameSetting *Npc::fMaxWalkSpeed;
    const ESM::GameSetting *Npc::fEncumberedMoveEffect;
    const ESM::GameSetting *Npc::fSneakSpeedMultiplier;
    const ESM::GameSetting *Npc::fAthleticsRunBonus;
    const ESM::GameSetting *Npc::fBaseRunMultiplier;
    const ESM::GameSetting *Npc::fMinFlySpeed;
    const ESM::GameSetting *Npc::fMaxFlySpeed;
    const ESM::GameSetting *Npc::fSwimRunBase;
    const ESM::GameSetting *Npc::fSwimRunAthleticsMult;
    const ESM::GameSetting *Npc::fJumpEncumbranceBase;
    const ESM::GameSetting *Npc::fJumpEncumbranceMultiplier;
    const ESM::GameSetting *Npc::fJumpAcrobaticsBase;
    const ESM::GameSetting *Npc::fJumpAcroMultiplier;
    const ESM::GameSetting *Npc::fJumpRunMultiplier;
    const ESM::GameSetting *Npc::fWereWolfRunMult;
}

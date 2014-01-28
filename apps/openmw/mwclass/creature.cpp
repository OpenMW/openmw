
#include "creature.hpp"

#include <components/esm/loadcrea.hpp>

#include "../mwmechanics/creaturestats.hpp"
#include "../mwmechanics/magiceffects.hpp"
#include "../mwmechanics/movement.hpp"
#include "../mwmechanics/spellcasting.hpp"

#include "../mwbase/environment.hpp"
#include "../mwbase/mechanicsmanager.hpp"
#include "../mwbase/windowmanager.hpp"
#include "../mwbase/world.hpp"
#include "../mwbase/soundmanager.hpp"

#include "../mwworld/ptr.hpp"
#include "../mwworld/actiontalk.hpp"
#include "../mwworld/actionopen.hpp"
#include "../mwworld/failedaction.hpp"
#include "../mwworld/customdata.hpp"
#include "../mwworld/containerstore.hpp"
#include "../mwworld/physicssystem.hpp"

#include "../mwrender/renderinginterface.hpp"
#include "../mwrender/actors.hpp"

#include "../mwgui/tooltips.hpp"

#include "../mwworld/inventorystore.hpp"

#include "../mwmechanics/npcstats.hpp"
#include "../mwmechanics/combat.hpp"

namespace
{
    struct CustomData : public MWWorld::CustomData
    {
        MWMechanics::CreatureStats mCreatureStats;
        MWWorld::ContainerStore* mContainerStore; // may be InventoryStore for some creatures
        MWMechanics::Movement mMovement;

        virtual MWWorld::CustomData *clone() const;

        CustomData() : mContainerStore(0) {}
        virtual ~CustomData() { delete mContainerStore; }
    };

    MWWorld::CustomData *CustomData::clone() const
    {
        CustomData* cloned = new CustomData (*this);
        cloned->mContainerStore = mContainerStore->clone();
        return cloned;
    }
}

namespace MWClass
{
    void Creature::ensureCustomData (const MWWorld::Ptr& ptr) const
    {
        if (!ptr.getRefData().getCustomData())
        {
            std::auto_ptr<CustomData> data (new CustomData);

            static bool inited = false;
            if(!inited)
            {
                const MWBase::World *world = MWBase::Environment::get().getWorld();
                const MWWorld::Store<ESM::GameSetting> &gmst = world->getStore().get<ESM::GameSetting>();

                fMinWalkSpeedCreature = gmst.find("fMinWalkSpeedCreature");
                fMaxWalkSpeedCreature = gmst.find("fMaxWalkSpeedCreature");
                fEncumberedMoveEffect = gmst.find("fEncumberedMoveEffect");
                fSneakSpeedMultiplier = gmst.find("fSneakSpeedMultiplier");
                fAthleticsRunBonus = gmst.find("fAthleticsRunBonus");
                fBaseRunMultiplier = gmst.find("fBaseRunMultiplier");
                fMinFlySpeed = gmst.find("fMinFlySpeed");
                fMaxFlySpeed = gmst.find("fMaxFlySpeed");
                fSwimRunBase = gmst.find("fSwimRunBase");
                fSwimRunAthleticsMult = gmst.find("fSwimRunAthleticsMult");
                fKnockDownMult = gmst.find("fKnockDownMult");
                iKnockDownOddsMult = gmst.find("iKnockDownOddsMult");
                iKnockDownOddsBase = gmst.find("iKnockDownOddsBase");

                inited = true;
            }

            MWWorld::LiveCellRef<ESM::Creature> *ref = ptr.get<ESM::Creature>();

            // creature stats
            data->mCreatureStats.setAttribute(ESM::Attribute::Strength, ref->mBase->mData.mStrength);
            data->mCreatureStats.setAttribute(ESM::Attribute::Intelligence, ref->mBase->mData.mIntelligence);
            data->mCreatureStats.setAttribute(ESM::Attribute::Willpower, ref->mBase->mData.mWillpower);
            data->mCreatureStats.setAttribute(ESM::Attribute::Agility, ref->mBase->mData.mAgility);
            data->mCreatureStats.setAttribute(ESM::Attribute::Speed, ref->mBase->mData.mSpeed);
            data->mCreatureStats.setAttribute(ESM::Attribute::Endurance, ref->mBase->mData.mEndurance);
            data->mCreatureStats.setAttribute(ESM::Attribute::Personality, ref->mBase->mData.mPersonality);
            data->mCreatureStats.setAttribute(ESM::Attribute::Luck, ref->mBase->mData.mLuck);
            data->mCreatureStats.setHealth (ref->mBase->mData.mHealth);
            data->mCreatureStats.setMagicka (ref->mBase->mData.mMana);
            data->mCreatureStats.setFatigue (ref->mBase->mData.mFatigue);

            data->mCreatureStats.setLevel(ref->mBase->mData.mLevel);

            data->mCreatureStats.getAiSequence().fill(ref->mBase->mAiPackage);

            data->mCreatureStats.setAiSetting (MWMechanics::CreatureStats::AI_Hello, ref->mBase->mAiData.mHello);
            data->mCreatureStats.setAiSetting (MWMechanics::CreatureStats::AI_Fight, ref->mBase->mAiData.mFight);
            data->mCreatureStats.setAiSetting (MWMechanics::CreatureStats::AI_Flee, ref->mBase->mAiData.mFlee);
            data->mCreatureStats.setAiSetting (MWMechanics::CreatureStats::AI_Alarm, ref->mBase->mAiData.mAlarm);

            // spells
            for (std::vector<std::string>::const_iterator iter (ref->mBase->mSpells.mList.begin());
                iter!=ref->mBase->mSpells.mList.end(); ++iter)
                data->mCreatureStats.getSpells().add (*iter);

            // inventory
            if (ref->mBase->mFlags & ESM::Creature::Weapon)
                data->mContainerStore = new MWWorld::InventoryStore();
            else
                data->mContainerStore = new MWWorld::ContainerStore();

            // store
            ptr.getRefData().setCustomData (data.release());

            getContainerStore(ptr).fill(ref->mBase->mInventory, getId(ptr), "",
                                       MWBase::Environment::get().getWorld()->getStore());

            // TODO: this is not quite correct, in vanilla the merchant's gold pool is not available in his inventory.
            // (except for gold you gave him)
            getContainerStore(ptr).add(MWWorld::ContainerStore::sGoldId, ref->mBase->mData.mGold, ptr);

            if (ref->mBase->mFlags & ESM::Creature::Weapon)
                getInventoryStore(ptr).autoEquip(ptr);
        }
    }

    std::string Creature::getId (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM::Creature> *ref =
            ptr.get<ESM::Creature>();

        return ref->mBase->mId;
    }

    void Creature::adjustPosition(const MWWorld::Ptr& ptr) const
    {
        MWBase::Environment::get().getWorld()->adjustPosition(ptr);
    }

    void Creature::insertObjectRendering (const MWWorld::Ptr& ptr, MWRender::RenderingInterface& renderingInterface) const
    {
        MWWorld::LiveCellRef<ESM::Creature> *ref = ptr.get<ESM::Creature>();

        MWRender::Actors& actors = renderingInterface.getActors();
        actors.insertCreature(ptr, ref->mBase->mFlags & ESM::Creature::Weapon);
    }

    void Creature::insertObject(const MWWorld::Ptr& ptr, MWWorld::PhysicsSystem& physics) const
    {
        const std::string model = getModel(ptr);
        if(!model.empty())
            physics.addActor(ptr);
        MWBase::Environment::get().getMechanicsManager()->add(ptr);
    }

    std::string Creature::getModel(const MWWorld::Ptr &ptr) const
    {
        MWWorld::LiveCellRef<ESM::Creature> *ref =
            ptr.get<ESM::Creature>();
        assert (ref->mBase != NULL);

        const std::string &model = ref->mBase->mModel;
        if (!model.empty()) {
            return "meshes\\" + model;
        }
        return "";
    }

    std::string Creature::getName (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM::Creature> *ref =
            ptr.get<ESM::Creature>();

        return ref->mBase->mName;
    }

    MWMechanics::CreatureStats& Creature::getCreatureStats (const MWWorld::Ptr& ptr) const
    {
        ensureCustomData (ptr);

        return dynamic_cast<CustomData&> (*ptr.getRefData().getCustomData()).mCreatureStats;
    }


    void Creature::hit(const MWWorld::Ptr& ptr, int type) const
    {
        MWWorld::LiveCellRef<ESM::Creature> *ref =
            ptr.get<ESM::Creature>();
        const MWWorld::Store<ESM::GameSetting> &gmst = MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>();
        MWMechanics::CreatureStats &stats = getCreatureStats(ptr);

        // Get the weapon used (if hand-to-hand, weapon = inv.end())
        MWWorld::Ptr weapon;
        if (ptr.getClass().hasInventoryStore(ptr))
        {
            MWWorld::InventoryStore &inv = getInventoryStore(ptr);
            MWWorld::ContainerStoreIterator weaponslot = inv.getSlot(MWWorld::InventoryStore::Slot_CarriedRight);
            if (weaponslot != inv.end() && weaponslot->getTypeName() == typeid(ESM::Weapon).name())
                weapon = *weaponslot;
        }

        // Reduce fatigue
        // somewhat of a guess, but using the weapon weight makes sense
        const float fFatigueAttackBase = gmst.find("fFatigueAttackBase")->getFloat();
        const float fFatigueAttackMult = gmst.find("fFatigueAttackMult")->getFloat();
        const float fWeaponFatigueMult = gmst.find("fWeaponFatigueMult")->getFloat();
        MWMechanics::DynamicStat<float> fatigue = stats.getFatigue();
        const float normalizedEncumbrance = getEncumbrance(ptr) / getCapacity(ptr);
        float fatigueLoss = fFatigueAttackBase + normalizedEncumbrance * fFatigueAttackMult;
        if (!weapon.isEmpty())
            fatigueLoss += weapon.getClass().getWeight(weapon) * stats.getAttackStrength() * fWeaponFatigueMult;
        fatigue.setCurrent(fatigue.getCurrent() - fatigueLoss);
        stats.setFatigue(fatigue);

        // TODO: where is the distance defined?
        float dist = 200.f;
        if (!weapon.isEmpty())
        {
            const float fCombatDistance = gmst.find("fCombatDistance")->getFloat();
            dist = fCombatDistance * weapon.get<ESM::Weapon>()->mBase->mData.mReach;
        }
        std::pair<MWWorld::Ptr, Ogre::Vector3> result = MWBase::Environment::get().getWorld()->getHitContact(ptr, dist);
        if (result.first.isEmpty())
            return; // Didn't hit anything

        MWWorld::Ptr victim = result.first;

        if (!victim.getClass().isActor())
            return; // Can't hit non-actors

        Ogre::Vector3 hitPosition = result.second;

        MWMechanics::CreatureStats &otherstats = victim.getClass().getCreatureStats(victim);
        const MWMechanics::MagicEffects &mageffects = stats.getMagicEffects();
        float hitchance = ref->mBase->mData.mCombat +
                          (stats.getAttribute(ESM::Attribute::Agility).getModified() / 5.0f) +
                          (stats.getAttribute(ESM::Attribute::Luck).getModified() / 10.0f);
        hitchance *= stats.getFatigueTerm();
        hitchance += mageffects.get(ESM::MagicEffect::FortifyAttack).mMagnitude -
                     mageffects.get(ESM::MagicEffect::Blind).mMagnitude;
        hitchance -= otherstats.getEvasion();

        if((::rand()/(RAND_MAX+1.0)) > hitchance/100.0f)
        {
            victim.getClass().onHit(victim, 0.0f, false, MWWorld::Ptr(), ptr, false);
            return;
        }

        int min,max;
        switch (type)
        {
        case 0:
            min = ref->mBase->mData.mAttack[0];
            max = ref->mBase->mData.mAttack[1];
            break;
        case 1:
            min = ref->mBase->mData.mAttack[2];
            max = ref->mBase->mData.mAttack[3];
            break;
        case 2:
        default:
            min = ref->mBase->mData.mAttack[4];
            max = ref->mBase->mData.mAttack[5];
            break;
        }

        // I think this should be random, since attack1-3 animations don't have an attack strength like NPCs do
        float damage = min + (max - min) * ::rand()/(RAND_MAX+1.0);

        if (!weapon.isEmpty())
        {
            const bool weaphashealth = get(weapon).hasItemHealth(weapon);
            const unsigned char *attack = NULL;
            if(type == ESM::Weapon::AT_Chop)
                attack = weapon.get<ESM::Weapon>()->mBase->mData.mChop;
            else if(type == ESM::Weapon::AT_Slash)
                attack = weapon.get<ESM::Weapon>()->mBase->mData.mSlash;
            else if(type == ESM::Weapon::AT_Thrust)
                attack = weapon.get<ESM::Weapon>()->mBase->mData.mThrust;
            if(attack)
            {
                float weaponDamage = attack[0] + ((attack[1]-attack[0])*stats.getAttackStrength());
                weaponDamage *= 0.5f + (stats.getAttribute(ESM::Attribute::Luck).getModified() / 100.0f);
                if(weaphashealth)
                {
                    int weapmaxhealth = weapon.get<ESM::Weapon>()->mBase->mData.mHealth;
                    if(weapon.getCellRef().mCharge == -1)
                        weapon.getCellRef().mCharge = weapmaxhealth;
                    weaponDamage *= float(weapon.getCellRef().mCharge) / weapmaxhealth;
                }

                if (!MWBase::Environment::get().getWorld()->getGodModeState())
                    weapon.getCellRef().mCharge -= std::min(std::max(1,
                        (int)(damage * gmst.find("fWeaponDamageMult")->getFloat())), weapon.getCellRef().mCharge);

                // Weapon broken? unequip it
                if (weapon.getCellRef().mCharge == 0)
                    weapon = *getInventoryStore(ptr).unequipItem(weapon, ptr);

                damage += weaponDamage;
            }

            // Apply "On hit" enchanted weapons
            std::string enchantmentName = !weapon.isEmpty() ? weapon.getClass().getEnchantment(weapon) : "";
            if (!enchantmentName.empty())
            {
                const ESM::Enchantment* enchantment = MWBase::Environment::get().getWorld()->getStore().get<ESM::Enchantment>().find(
                            enchantmentName);
                if (enchantment->mData.mType == ESM::Enchantment::WhenStrikes)
                {
                    MWMechanics::CastSpell cast(ptr, victim);
                    cast.mHitPosition = hitPosition;
                    cast.cast(weapon);
                }
            }
        }

        if (!weapon.isEmpty() && MWMechanics::blockMeleeAttack(ptr, victim, weapon, damage))
            damage = 0;

        if (damage > 0)
            MWBase::Environment::get().getWorld()->spawnBloodEffect(victim, hitPosition);

        victim.getClass().onHit(victim, damage, true, weapon, ptr, true);
    }

    void Creature::onHit(const MWWorld::Ptr &ptr, float damage, bool ishealth, const MWWorld::Ptr &object, const MWWorld::Ptr &attacker, bool successful) const
    {
        // NOTE: 'object' and/or 'attacker' may be empty.

        if(!successful)
        {
            // TODO: Handle HitAttemptOnMe script function

            // Missed
            MWBase::Environment::get().getSoundManager()->playSound3D(ptr, "miss", 1.0f, 1.0f);
            return;
        }

        if(!object.isEmpty())
            getCreatureStats(ptr).setLastHitObject(MWWorld::Class::get(object).getId(object));

        if(!attacker.isEmpty() && attacker.getRefData().getHandle() == "player")
        {
            const std::string &script = ptr.get<ESM::Creature>()->mBase->mScript;
            /* Set the OnPCHitMe script variable. The script is responsible for clearing it. */
            if(!script.empty())
                ptr.getRefData().getLocals().setVarByInt(script, "onpchitme", 1);
        }

        if (damage > 0.0f && !object.isEmpty())
            MWMechanics::resistNormalWeapon(ptr, attacker, object, damage);

        if (damage > 0.f)
        {
            // Check for knockdown
            float agilityTerm = getCreatureStats(ptr).getAttribute(ESM::Attribute::Agility).getModified() * fKnockDownMult->getFloat();
            float knockdownTerm = getCreatureStats(ptr).getAttribute(ESM::Attribute::Agility).getModified()
                    * iKnockDownOddsMult->getInt() * 0.01 + iKnockDownOddsBase->getInt();
            int roll = std::rand()/ (static_cast<double> (RAND_MAX) + 1) * 100; // [0, 99]
            if (ishealth && agilityTerm <= damage && knockdownTerm <= roll)
            {
                getCreatureStats(ptr).setKnockedDown(true);

            }
            else
                getCreatureStats(ptr).setHitRecovery(true); // Is this supposed to always occur?

            if(ishealth)
            {
                MWBase::Environment::get().getSoundManager()->playSound3D(ptr, "Health Damage", 1.0f, 1.0f);
                float health = getCreatureStats(ptr).getHealth().getCurrent() - damage;
                setActorHealth(ptr, health, attacker);
            }
            else
            {
                MWMechanics::DynamicStat<float> fatigue(getCreatureStats(ptr).getFatigue());
                fatigue.setCurrent(fatigue.getCurrent() - damage, true);
                getCreatureStats(ptr).setFatigue(fatigue);
            }
        }
    }

    void Creature::block(const MWWorld::Ptr &ptr) const
    {
        MWWorld::InventoryStore& inv = getInventoryStore(ptr);
        MWWorld::ContainerStoreIterator shield = inv.getSlot(MWWorld::InventoryStore::Slot_CarriedLeft);
        if (shield == inv.end())
            return;

        MWBase::SoundManager *sndMgr = MWBase::Environment::get().getSoundManager();
        switch(shield->getClass().getEquipmentSkill(*shield))
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
            default:
                return;
        }
    }

    void Creature::setActorHealth(const MWWorld::Ptr& ptr, float health, const MWWorld::Ptr& attacker) const
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


    boost::shared_ptr<MWWorld::Action> Creature::activate (const MWWorld::Ptr& ptr,
        const MWWorld::Ptr& actor) const
    {
        if(get(actor).isNpc() && get(actor).getNpcStats(actor).isWerewolf())
        {
            const MWWorld::ESMStore &store = MWBase::Environment::get().getWorld()->getStore();
            const ESM::Sound *sound = store.get<ESM::Sound>().searchRandom("WolfCreature");

            boost::shared_ptr<MWWorld::Action> action(new MWWorld::FailedAction("#{sWerewolfRefusal}"));
            if(sound) action->setSound(sound->mId);

            return action;
        }

        if(getCreatureStats(ptr).isDead())
            return boost::shared_ptr<MWWorld::Action>(new MWWorld::ActionOpen(ptr, true));
        return boost::shared_ptr<MWWorld::Action>(new MWWorld::ActionTalk(ptr));
    }

    MWWorld::ContainerStore& Creature::getContainerStore (const MWWorld::Ptr& ptr) const
    {
        ensureCustomData (ptr);

        return *dynamic_cast<CustomData&> (*ptr.getRefData().getCustomData()).mContainerStore;
    }

    MWWorld::InventoryStore& Creature::getInventoryStore(const MWWorld::Ptr &ptr) const
    {
        MWWorld::LiveCellRef<ESM::Creature> *ref = ptr.get<ESM::Creature>();

        if (ref->mBase->mFlags & ESM::Creature::Weapon)
            return dynamic_cast<MWWorld::InventoryStore&>(getContainerStore(ptr));
        else
            throw std::runtime_error("this creature has no inventory store");
    }

    bool Creature::hasInventoryStore(const MWWorld::Ptr &ptr) const
    {
        MWWorld::LiveCellRef<ESM::Creature> *ref = ptr.get<ESM::Creature>();

        return (ref->mBase->mFlags & ESM::Creature::Weapon);
    }

    std::string Creature::getScript (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM::Creature> *ref = ptr.get<ESM::Creature>();

        return ref->mBase->mScript;
    }

    bool Creature::isEssential (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM::Creature> *ref =
            ptr.get<ESM::Creature>();

        return ref->mBase->mFlags & ESM::Creature::Essential;
    }

    void Creature::registerSelf()
    {
        boost::shared_ptr<Class> instance (new Creature);

        registerClass (typeid (ESM::Creature).name(), instance);
    }

    bool Creature::hasToolTip (const MWWorld::Ptr& ptr) const
    {
        /// \todo We don't want tooltips for Creatures in combat mode.

        return true;
    }

    float Creature::getSpeed(const MWWorld::Ptr &ptr) const
    {
        MWMechanics::CreatureStats& stats = getCreatureStats(ptr);

        float walkSpeed = fMinWalkSpeedCreature->getFloat() + 0.01 * stats.getAttribute(ESM::Attribute::Speed).getModified()
                * (fMaxWalkSpeedCreature->getFloat() - fMinWalkSpeedCreature->getFloat());

        const MWBase::World *world = MWBase::Environment::get().getWorld();
        const MWMechanics::MagicEffects &mageffects = stats.getMagicEffects();

        const float normalizedEncumbrance = getEncumbrance(ptr) / getCapacity(ptr);

        bool running = ptr.getClass().getCreatureStats(ptr).getStance(MWMechanics::CreatureStats::Stance_Run);

        float runSpeed = walkSpeed*(0.01f * getSkill(ptr, ESM::Skill::Athletics) *
                                    fAthleticsRunBonus->getFloat() + fBaseRunMultiplier->getFloat());

        float moveSpeed;
        if(normalizedEncumbrance >= 1.0f)
            moveSpeed = 0.0f;
        else if(mageffects.get(ESM::MagicEffect::Levitate).mMagnitude > 0 &&
                world->isLevitationEnabled())
        {
            float flySpeed = 0.01f*(stats.getAttribute(ESM::Attribute::Speed).getModified() +
                                    mageffects.get(ESM::MagicEffect::Levitate).mMagnitude);
            flySpeed = fMinFlySpeed->getFloat() + flySpeed*(fMaxFlySpeed->getFloat() - fMinFlySpeed->getFloat());
            flySpeed *= 1.0f - fEncumberedMoveEffect->getFloat() * normalizedEncumbrance;
            flySpeed = std::max(0.0f, flySpeed);
            moveSpeed = flySpeed;
        }
        else if(world->isSwimming(ptr))
        {
            float swimSpeed = walkSpeed;
            if(running)
                swimSpeed = runSpeed;
            swimSpeed *= 1.0f + 0.01f * mageffects.get(ESM::MagicEffect::SwiftSwim).mMagnitude;
            swimSpeed *= fSwimRunBase->getFloat() + 0.01f*getSkill(ptr, ESM::Skill::Athletics) *
                                                    fSwimRunAthleticsMult->getFloat();
            moveSpeed = swimSpeed;
        }
        else if(running)
            moveSpeed = runSpeed;
        else
            moveSpeed = walkSpeed;
        if(getMovementSettings(ptr).mPosition[0] != 0 && getMovementSettings(ptr).mPosition[1] == 0)
            moveSpeed *= 0.75f;

        return moveSpeed;
    }

    MWMechanics::Movement& Creature::getMovementSettings (const MWWorld::Ptr& ptr) const
    {
        ensureCustomData (ptr);

        return dynamic_cast<CustomData&> (*ptr.getRefData().getCustomData()).mMovement;
    }

    Ogre::Vector3 Creature::getMovementVector (const MWWorld::Ptr& ptr) const
    {
        MWMechanics::Movement &movement = getMovementSettings(ptr);
        Ogre::Vector3 vec(movement.mPosition);
        movement.mPosition[0] = 0.0f;
        movement.mPosition[1] = 0.0f;
        movement.mPosition[2] = 0.0f;
        return vec;
    }

    Ogre::Vector3 Creature::getRotationVector (const MWWorld::Ptr& ptr) const
    {
        MWMechanics::Movement &movement = getMovementSettings(ptr);
        Ogre::Vector3 vec(movement.mRotation);
        movement.mRotation[0] = 0.0f;
        movement.mRotation[1] = 0.0f;
        movement.mRotation[2] = 0.0f;
        return vec;
    }

    MWGui::ToolTipInfo Creature::getToolTipInfo (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM::Creature> *ref =
            ptr.get<ESM::Creature>();

        MWGui::ToolTipInfo info;
        info.caption = ref->mBase->mName;

        std::string text;
        if (MWBase::Environment::get().getWindowManager()->getFullHelp())
            text += MWGui::ToolTips::getMiscString(ref->mBase->mScript, "Script");
        info.text = text;

        return info;
    }

    float Creature::getArmorRating (const MWWorld::Ptr& ptr) const
    {
        /// \todo add Shield magic effect magnitude here, controlled by a GMST (Vanilla vs MCP behaviour)
        return 0.f;
    }

    float Creature::getCapacity (const MWWorld::Ptr& ptr) const
    {
        const MWMechanics::CreatureStats& stats = getCreatureStats (ptr);
        return stats.getAttribute(0).getModified()*5;
    }

    float Creature::getEncumbrance (const MWWorld::Ptr& ptr) const
    {
        float weight = getContainerStore (ptr).getWeight();

        const MWMechanics::CreatureStats& stats = getCreatureStats (ptr);

        weight -= stats.getMagicEffects().get (MWMechanics::EffectKey (ESM::MagicEffect::Feather)).mMagnitude;

        weight += stats.getMagicEffects().get (MWMechanics::EffectKey (ESM::MagicEffect::Burden)).mMagnitude;

        if (weight<0)
            weight = 0;

        return weight;
    }


    int Creature::getServices(const MWWorld::Ptr &actor) const
    {
        MWWorld::LiveCellRef<ESM::Creature>* ref = actor.get<ESM::Creature>();
        if (ref->mBase->mHasAI)
            return ref->mBase->mAiData.mServices;
        else
            return 0;
    }

    bool Creature::isPersistent(const MWWorld::Ptr &actor) const
    {
        MWWorld::LiveCellRef<ESM::Creature>* ref = actor.get<ESM::Creature>();
        return ref->mBase->mPersistent;
    }

    std::string Creature::getSoundIdFromSndGen(const MWWorld::Ptr &ptr, const std::string &name) const
    {
        const MWWorld::Store<ESM::SoundGenerator> &store = MWBase::Environment::get().getWorld()->getStore().get<ESM::SoundGenerator>();

        int type = getSndGenTypeFromName(ptr, name);
        if(type >= 0)
        {
            std::vector<const ESM::SoundGenerator*> sounds;
            sounds.reserve(8);

            std::string ptrid = Creature::getId(ptr);
            MWWorld::Store<ESM::SoundGenerator>::iterator sound = store.begin();
            while(sound != store.end())
            {
                if(type == sound->mType && !sound->mCreature.empty() &&
                   Misc::StringUtils::ciEqual(ptrid.substr(0, sound->mCreature.size()),
                                              sound->mCreature))
                    sounds.push_back(&*sound);
                ++sound;
            }
            if(!sounds.empty())
                return sounds[(int)(rand()/(RAND_MAX+1.0)*sounds.size())]->mSound;
        }

        return "";
    }

    MWWorld::Ptr
    Creature::copyToCellImpl(const MWWorld::Ptr &ptr, MWWorld::CellStore &cell) const
    {
        MWWorld::LiveCellRef<ESM::Creature> *ref =
            ptr.get<ESM::Creature>();

        return MWWorld::Ptr(&cell.mCreatures.insert(*ref), &cell);
    }

    bool Creature::isFlying(const MWWorld::Ptr &ptr) const
    {
        MWWorld::LiveCellRef<ESM::Creature> *ref =
            ptr.get<ESM::Creature>();

        return ref->mBase->mFlags & ESM::Creature::Flies;
    }

    int Creature::getSndGenTypeFromName(const MWWorld::Ptr &ptr, const std::string &name)
    {
        if(name == "left")
        {
            MWBase::World *world = MWBase::Environment::get().getWorld();
            Ogre::Vector3 pos(ptr.getRefData().getPosition().pos);
            if(world->isUnderwater(ptr.getCell(), pos))
                return 2;
            if(world->isOnGround(ptr))
                return 0;
            return -1;
        }
        if(name == "right")
        {
            MWBase::World *world = MWBase::Environment::get().getWorld();
            Ogre::Vector3 pos(ptr.getRefData().getPosition().pos);
            if(world->isUnderwater(ptr.getCell(), pos))
                return 3;
            if(world->isOnGround(ptr))
                return 1;
            return -1;
        }
        if(name == "swimleft")
            return 2;
        if(name == "swimright")
            return 3;
        if(name == "moan")
            return 4;
        if(name == "roar")
            return 5;
        if(name == "scream")
            return 6;
        if(name == "land")
            return 7;

        throw std::runtime_error(std::string("Unexpected soundgen type: ")+name);
    }

    int Creature::getSkill(const MWWorld::Ptr &ptr, int skill) const
    {
        MWWorld::LiveCellRef<ESM::Creature> *ref =
            ptr.get<ESM::Creature>();

        const ESM::Skill* skillRecord = MWBase::Environment::get().getWorld()->getStore().get<ESM::Skill>().find(skill);

        switch (skillRecord->mData.mSpecialization)
        {
        case ESM::Class::Combat:
            return ref->mBase->mData.mCombat;
        case ESM::Class::Magic:
            return ref->mBase->mData.mMagic;
        case ESM::Class::Stealth:
            return ref->mBase->mData.mStealth;
        default:
            throw std::runtime_error("invalid specialisation");
        }
    }

    int Creature::getBloodTexture(const MWWorld::Ptr &ptr) const
    {
        MWWorld::LiveCellRef<ESM::Creature> *ref = ptr.get<ESM::Creature>();

        if (ref->mBase->mFlags & ESM::Creature::Skeleton)
            return 1;
        if (ref->mBase->mFlags & ESM::Creature::Metal)
            return 2;
        return 0;
    }

    const ESM::GameSetting* Creature::fMinWalkSpeedCreature;
    const ESM::GameSetting* Creature::fMaxWalkSpeedCreature;
    const ESM::GameSetting *Creature::fEncumberedMoveEffect;
    const ESM::GameSetting *Creature::fSneakSpeedMultiplier;
    const ESM::GameSetting *Creature::fAthleticsRunBonus;
    const ESM::GameSetting *Creature::fBaseRunMultiplier;
    const ESM::GameSetting *Creature::fMinFlySpeed;
    const ESM::GameSetting *Creature::fMaxFlySpeed;
    const ESM::GameSetting *Creature::fSwimRunBase;
    const ESM::GameSetting *Creature::fSwimRunAthleticsMult;
    const ESM::GameSetting *Creature::fKnockDownMult;
    const ESM::GameSetting *Creature::iKnockDownOddsMult;
    const ESM::GameSetting *Creature::iKnockDownOddsBase;

}


#include "creature.hpp"

#include <openengine/misc/rng.hpp>

#include <components/esm/loadcrea.hpp>
#include <components/esm/creaturestate.hpp>

#include "../mwmechanics/creaturestats.hpp"
#include "../mwmechanics/magiceffects.hpp"
#include "../mwmechanics/movement.hpp"
#include "../mwmechanics/disease.hpp"
#include "../mwmechanics/spellcasting.hpp"
#include "../mwmechanics/difficultyscaling.hpp"

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
#include "../mwworld/cellstore.hpp"

#include "../mwrender/renderinginterface.hpp"
#include "../mwrender/actors.hpp"

#include "../mwgui/tooltips.hpp"

#include "../mwworld/inventorystore.hpp"

#include "../mwmechanics/npcstats.hpp"
#include "../mwmechanics/combat.hpp"

namespace
{
    struct CreatureCustomData : public MWWorld::CustomData
    {
        MWMechanics::CreatureStats mCreatureStats;
        MWWorld::ContainerStore* mContainerStore; // may be InventoryStore for some creatures
        MWMechanics::Movement mMovement;

        virtual MWWorld::CustomData *clone() const;

        CreatureCustomData() : mContainerStore(0) {}
        virtual ~CreatureCustomData() { delete mContainerStore; }
    };

    MWWorld::CustomData *CreatureCustomData::clone() const
    {
        CreatureCustomData* cloned = new CreatureCustomData (*this);
        cloned->mContainerStore = mContainerStore->clone();
        return cloned;
    }
}

namespace MWClass
{
    const Creature::GMST& Creature::getGmst()
    {
        static GMST gmst;
        static bool inited = false;
        if (!inited)
        {
            const MWBase::World *world = MWBase::Environment::get().getWorld();
            const MWWorld::Store<ESM::GameSetting> &store = world->getStore().get<ESM::GameSetting>();
            gmst.fMinWalkSpeedCreature = store.find("fMinWalkSpeedCreature");
            gmst.fMaxWalkSpeedCreature = store.find("fMaxWalkSpeedCreature");
            gmst.fEncumberedMoveEffect = store.find("fEncumberedMoveEffect");
            gmst.fSneakSpeedMultiplier = store.find("fSneakSpeedMultiplier");
            gmst.fAthleticsRunBonus = store.find("fAthleticsRunBonus");
            gmst.fBaseRunMultiplier = store.find("fBaseRunMultiplier");
            gmst.fMinFlySpeed = store.find("fMinFlySpeed");
            gmst.fMaxFlySpeed = store.find("fMaxFlySpeed");
            gmst.fSwimRunBase = store.find("fSwimRunBase");
            gmst.fSwimRunAthleticsMult = store.find("fSwimRunAthleticsMult");
            gmst.fKnockDownMult = store.find("fKnockDownMult");
            gmst.iKnockDownOddsMult = store.find("iKnockDownOddsMult");
            gmst.iKnockDownOddsBase = store.find("iKnockDownOddsBase");
            inited = true;
        }
        return gmst;
    }

    void Creature::ensureCustomData (const MWWorld::Ptr& ptr) const
    {
        if (!ptr.getRefData().getCustomData())
        {
            std::auto_ptr<CreatureCustomData> data (new CreatureCustomData);

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
            data->mCreatureStats.setHealth(static_cast<float>(ref->mBase->mData.mHealth));
            data->mCreatureStats.setMagicka(static_cast<float>(ref->mBase->mData.mMana));
            data->mCreatureStats.setFatigue(static_cast<float>(ref->mBase->mData.mFatigue));

            data->mCreatureStats.setLevel(ref->mBase->mData.mLevel);

            data->mCreatureStats.getAiSequence().fill(ref->mBase->mAiPackage);

            data->mCreatureStats.setAiSetting (MWMechanics::CreatureStats::AI_Hello, ref->mBase->mAiData.mHello);
            data->mCreatureStats.setAiSetting (MWMechanics::CreatureStats::AI_Fight, ref->mBase->mAiData.mFight);
            data->mCreatureStats.setAiSetting (MWMechanics::CreatureStats::AI_Flee, ref->mBase->mAiData.mFlee);
            data->mCreatureStats.setAiSetting (MWMechanics::CreatureStats::AI_Alarm, ref->mBase->mAiData.mAlarm);

            // spells
            for (std::vector<std::string>::const_iterator iter (ref->mBase->mSpells.mList.begin());
                iter!=ref->mBase->mSpells.mList.end(); ++iter)
            {
                if (const ESM::Spell* spell = MWBase::Environment::get().getWorld()->getStore().get<ESM::Spell>().search(*iter))
                    data->mCreatureStats.getSpells().add (spell);
                else /// \todo add option to make this a fatal error message pop-up, but default to warning for vanilla compatibility
                    std::cerr << "Warning: ignoring nonexistent spell '" << *iter << "' on creature '" << ref->mBase->mId << "'" << std::endl;
            }

            // inventory
            if (ref->mBase->mFlags & ESM::Creature::Weapon)
                data->mContainerStore = new MWWorld::InventoryStore();
            else
                data->mContainerStore = new MWWorld::ContainerStore();

            data->mCreatureStats.setGoldPool(ref->mBase->mData.mGold);

            data->mCreatureStats.setNeedRecalcDynamicStats(false);

            // store
            ptr.getRefData().setCustomData(data.release());

            getContainerStore(ptr).fill(ref->mBase->mInventory, getId(ptr));

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

    void Creature::adjustPosition(const MWWorld::Ptr& ptr, bool force) const
    {
        MWBase::Environment::get().getWorld()->adjustPosition(ptr, force);
    }

    void Creature::insertObjectRendering (const MWWorld::Ptr& ptr, const std::string& model, MWRender::RenderingInterface& renderingInterface) const
    {
        MWWorld::LiveCellRef<ESM::Creature> *ref = ptr.get<ESM::Creature>();

        MWRender::Actors& actors = renderingInterface.getActors();
        actors.insertCreature(ptr, model, (ref->mBase->mFlags & ESM::Creature::Weapon) != 0);
    }

    void Creature::insertObject(const MWWorld::Ptr& ptr, const std::string& model, MWWorld::PhysicsSystem& physics) const
    {
        if(!model.empty())
        {
            physics.addActor(ptr, model);
            if (getCreatureStats(ptr).isDead())
                MWBase::Environment::get().getWorld()->enableActorCollision(ptr, false);
        }
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

        return dynamic_cast<CreatureCustomData&> (*ptr.getRefData().getCustomData()).mCreatureStats;
    }


    void Creature::hit(const MWWorld::Ptr& ptr, int type) const
    {
        MWWorld::LiveCellRef<ESM::Creature> *ref =
            ptr.get<ESM::Creature>();
        const MWWorld::Store<ESM::GameSetting> &gmst = MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>();
        MWMechanics::CreatureStats &stats = getCreatureStats(ptr);

        if (stats.getDrawState() != MWMechanics::DrawState_Weapon)
            return;

        // Get the weapon used (if hand-to-hand, weapon = inv.end())
        MWWorld::Ptr weapon;
        if (ptr.getClass().hasInventoryStore(ptr))
        {
            MWWorld::InventoryStore &inv = getInventoryStore(ptr);
            MWWorld::ContainerStoreIterator weaponslot = inv.getSlot(MWWorld::InventoryStore::Slot_CarriedRight);
            if (weaponslot != inv.end() && weaponslot->getTypeName() == typeid(ESM::Weapon).name())
                weapon = *weaponslot;
        }

        MWMechanics::applyFatigueLoss(ptr, weapon);

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

        float hitchance = MWMechanics::getHitChance(ptr, victim, ref->mBase->mData.mCombat);

        if(OEngine::Misc::Rng::roll0to99() >= hitchance)
        {
            victim.getClass().onHit(victim, 0.0f, false, MWWorld::Ptr(), ptr, false);
            MWMechanics::reduceWeaponCondition(0.f, false, weapon, ptr);
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

        float damage = min + (max - min) * stats.getAttackStrength();
        bool healthdmg = true;
        if (!weapon.isEmpty())
        {
            const unsigned char *attack = NULL;
            if(type == ESM::Weapon::AT_Chop)
                attack = weapon.get<ESM::Weapon>()->mBase->mData.mChop;
            else if(type == ESM::Weapon::AT_Slash)
                attack = weapon.get<ESM::Weapon>()->mBase->mData.mSlash;
            else if(type == ESM::Weapon::AT_Thrust)
                attack = weapon.get<ESM::Weapon>()->mBase->mData.mThrust;
            if(attack)
            {
                damage = attack[0] + ((attack[1]-attack[0])*stats.getAttackStrength());
                MWMechanics::adjustWeaponDamage(damage, weapon, ptr);
                MWMechanics::reduceWeaponCondition(damage, true, weapon, ptr);
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
        else if (isBipedal(ptr))
        {
            MWMechanics::getHandToHandDamage(ptr, victim, damage, healthdmg);
        }

        MWMechanics::applyElementalShields(ptr, victim);

        if (MWMechanics::blockMeleeAttack(ptr, victim, weapon, damage))
            damage = 0;

        if (damage > 0)
            MWBase::Environment::get().getWorld()->spawnBloodEffect(victim, hitPosition);

        MWMechanics::diseaseContact(victim, ptr);

        victim.getClass().onHit(victim, damage, healthdmg, weapon, ptr, true);
    }

    void Creature::onHit(const MWWorld::Ptr &ptr, float damage, bool ishealth, const MWWorld::Ptr &object, const MWWorld::Ptr &attacker, bool successful) const
    {
        // NOTE: 'object' and/or 'attacker' may be empty.

        if (!attacker.isEmpty() && !ptr.getClass().getCreatureStats(ptr).getAiSequence().isInCombat(attacker))
            getCreatureStats(ptr).setAttacked(true);

        // Self defense
        bool setOnPcHitMe = true; // Note OnPcHitMe is not set for friendly hits.
        
        // No retaliation for totally static creatures (they have no movement or attacks anyway)
        if (isMobile(ptr) && !attacker.isEmpty())
        {
            setOnPcHitMe = MWBase::Environment::get().getMechanicsManager()->actorAttacked(ptr, attacker);
        }

        if(!object.isEmpty())
            getCreatureStats(ptr).setLastHitAttemptObject(object.getClass().getId(object));

        if(!successful)
        {
            // Missed
            MWBase::Environment::get().getSoundManager()->playSound3D(ptr, "miss", 1.0f, 1.0f);
            return;
        }

        if(!object.isEmpty())
            getCreatureStats(ptr).setLastHitObject(object.getClass().getId(object));

        if(setOnPcHitMe && !attacker.isEmpty() && attacker == MWBase::Environment::get().getWorld()->getPlayerPtr())
        {
            const std::string &script = ptr.get<ESM::Creature>()->mBase->mScript;
            /* Set the OnPCHitMe script variable. The script is responsible for clearing it. */
            if(!script.empty())
                ptr.getRefData().getLocals().setVarByInt(script, "onpchitme", 1);
        }

        if (damage > 0.0f && !object.isEmpty())
            MWMechanics::resistNormalWeapon(ptr, attacker, object, damage);

        if (damage < 0.001f)
            damage = 0;

        if (damage > 0.f)
        {
            if (!attacker.isEmpty())
            {
                // Check for knockdown
                float agilityTerm = getCreatureStats(ptr).getAttribute(ESM::Attribute::Agility).getModified() * getGmst().fKnockDownMult->getFloat();
                float knockdownTerm = getCreatureStats(ptr).getAttribute(ESM::Attribute::Agility).getModified()
                        * getGmst().iKnockDownOddsMult->getInt() * 0.01f + getGmst().iKnockDownOddsBase->getInt();
                if (ishealth && agilityTerm <= damage && knockdownTerm <= OEngine::Misc::Rng::roll0to99())
                {
                    getCreatureStats(ptr).setKnockedDown(true);

                }
                else
                    getCreatureStats(ptr).setHitRecovery(true); // Is this supposed to always occur?
            }

            damage = std::max(1.f, damage);

            if(ishealth)
            {
                if (!attacker.isEmpty())
                    damage = scaleDamage(damage, attacker, ptr);

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
        if(actor.getClass().isNpc() && actor.getClass().getNpcStats(actor).isWerewolf())
        {
            const MWWorld::ESMStore &store = MWBase::Environment::get().getWorld()->getStore();
            const ESM::Sound *sound = store.get<ESM::Sound>().searchRandom("WolfCreature");

            boost::shared_ptr<MWWorld::Action> action(new MWWorld::FailedAction("#{sWerewolfRefusal}"));
            if(sound) action->setSound(sound->mId);

            return action;
        }

        if(getCreatureStats(ptr).isDead())
            return boost::shared_ptr<MWWorld::Action>(new MWWorld::ActionOpen(ptr, true));
        if(ptr.getClass().getCreatureStats(ptr).getAiSequence().isInCombat())
            return boost::shared_ptr<MWWorld::Action>(new MWWorld::FailedAction(""));
        return boost::shared_ptr<MWWorld::Action>(new MWWorld::ActionTalk(ptr));
    }

    MWWorld::ContainerStore& Creature::getContainerStore (const MWWorld::Ptr& ptr) const
    {
        ensureCustomData (ptr);

        return *dynamic_cast<CreatureCustomData&> (*ptr.getRefData().getCustomData()).mContainerStore;
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

        return (ref->mBase->mFlags & ESM::Creature::Weapon) != 0;
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

        return (ref->mBase->mFlags & ESM::Creature::Essential) != 0;
    }

    void Creature::registerSelf()
    {
        boost::shared_ptr<Class> instance (new Creature);

        registerClass (typeid (ESM::Creature).name(), instance);
    }

    bool Creature::hasToolTip (const MWWorld::Ptr& ptr) const
    {
        return !ptr.getClass().getCreatureStats(ptr).getAiSequence().isInCombat() || getCreatureStats(ptr).isDead();
    }

    float Creature::getSpeed(const MWWorld::Ptr &ptr) const
    {
        MWMechanics::CreatureStats& stats = getCreatureStats(ptr);
        const GMST& gmst = getGmst();

        float walkSpeed = gmst.fMinWalkSpeedCreature->getFloat() + 0.01f * stats.getAttribute(ESM::Attribute::Speed).getModified()
                * (gmst.fMaxWalkSpeedCreature->getFloat() - gmst.fMinWalkSpeedCreature->getFloat());

        const MWBase::World *world = MWBase::Environment::get().getWorld();
        const MWMechanics::MagicEffects &mageffects = stats.getMagicEffects();

        bool running = ptr.getClass().getCreatureStats(ptr).getStance(MWMechanics::CreatureStats::Stance_Run);

        // The Run speed difference for creatures comes from the animation speed difference (see runStateToWalkState in character.cpp)
        float runSpeed = walkSpeed;

        float moveSpeed;

        if(getEncumbrance(ptr) > getCapacity(ptr))
            moveSpeed = 0.0f;
        else if(canFly(ptr) || (mageffects.get(ESM::MagicEffect::Levitate).getMagnitude() > 0 &&
                world->isLevitationEnabled()))
        {
            float flySpeed = 0.01f*(stats.getAttribute(ESM::Attribute::Speed).getModified() +
                                    mageffects.get(ESM::MagicEffect::Levitate).getMagnitude());
            flySpeed = gmst.fMinFlySpeed->getFloat() + flySpeed*(gmst.fMaxFlySpeed->getFloat() - gmst.fMinFlySpeed->getFloat());
            const float normalizedEncumbrance = getNormalizedEncumbrance(ptr);
            flySpeed *= 1.0f - gmst.fEncumberedMoveEffect->getFloat() * normalizedEncumbrance;
            flySpeed = std::max(0.0f, flySpeed);
            moveSpeed = flySpeed;
        }
        else if(world->isSwimming(ptr))
        {
            float swimSpeed = walkSpeed;
            if(running)
                swimSpeed = runSpeed;
            swimSpeed *= 1.0f + 0.01f * mageffects.get(ESM::MagicEffect::SwiftSwim).getMagnitude();
            swimSpeed *= gmst.fSwimRunBase->getFloat() + 0.01f*getSkill(ptr, ESM::Skill::Athletics) *
                                                    gmst.fSwimRunAthleticsMult->getFloat();
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

        return dynamic_cast<CreatureCustomData&> (*ptr.getRefData().getCustomData()).mMovement;
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
        // Note this is currently unused. Creatures do not use armor mitigation.
        return getCreatureStats(ptr).getMagicEffects().get(ESM::MagicEffect::Shield).getMagnitude();
    }

    float Creature::getCapacity (const MWWorld::Ptr& ptr) const
    {
        const MWMechanics::CreatureStats& stats = getCreatureStats (ptr);
        return static_cast<float>(stats.getAttribute(0).getModified() * 5);
    }

    float Creature::getEncumbrance (const MWWorld::Ptr& ptr) const
    {
        float weight = getContainerStore (ptr).getWeight();

        const MWMechanics::CreatureStats& stats = getCreatureStats (ptr);

        weight -= stats.getMagicEffects().get (MWMechanics::EffectKey (ESM::MagicEffect::Feather)).getMagnitude();

        weight += stats.getMagicEffects().get (MWMechanics::EffectKey (ESM::MagicEffect::Burden)).getMagnitude();

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

            MWWorld::LiveCellRef<ESM::Creature>* ref = ptr.get<ESM::Creature>();

            const std::string& ourId = (ref->mBase->mOriginal.empty()) ? getId(ptr) : ref->mBase->mOriginal;

            MWWorld::Store<ESM::SoundGenerator>::iterator sound = store.begin();
            while(sound != store.end())
            {
                if (type == sound->mType && !sound->mCreature.empty() && (Misc::StringUtils::ciEqual(ourId, sound->mCreature)))
                    sounds.push_back(&*sound);
                ++sound;
            }
            if(!sounds.empty())
                return sounds[OEngine::Misc::Rng::rollDice(sounds.size())]->mSound;
        }

        if (type == ESM::SoundGenerator::Land)
            return "Body Fall Large";

        return "";
    }

    MWWorld::Ptr
    Creature::copyToCellImpl(const MWWorld::Ptr &ptr, MWWorld::CellStore &cell) const
    {
        MWWorld::LiveCellRef<ESM::Creature> *ref =
            ptr.get<ESM::Creature>();

        return MWWorld::Ptr(&cell.get<ESM::Creature>().insert(*ref), &cell);
    }

    bool Creature::isBipedal(const MWWorld::Ptr &ptr) const
    {
        MWWorld::LiveCellRef<ESM::Creature> *ref =
            ptr.get<ESM::Creature>();

        return ref->mBase->mFlags & ESM::Creature::Bipedal;
    }

    bool Creature::canFly(const MWWorld::Ptr &ptr) const
    {
        MWWorld::LiveCellRef<ESM::Creature> *ref =
            ptr.get<ESM::Creature>();

        return (ref->mBase->mFlags & ESM::Creature::Flies) != 0;
    }

    bool Creature::canSwim(const MWWorld::Ptr &ptr) const
    {
        MWWorld::LiveCellRef<ESM::Creature> *ref =
            ptr.get<ESM::Creature>();

        return ref->mBase->mFlags & ESM::Creature::Swims || ref->mBase->mFlags & ESM::Creature::Bipedal;
    }

    bool Creature::canWalk(const MWWorld::Ptr &ptr) const
    {
        MWWorld::LiveCellRef<ESM::Creature> *ref =
            ptr.get<ESM::Creature>();

        return ref->mBase->mFlags & ESM::Creature::Walks || ref->mBase->mFlags & ESM::Creature::Bipedal;
    }

    int Creature::getSndGenTypeFromName(const MWWorld::Ptr &ptr, const std::string &name)
    {
        if(name == "left")
        {
            MWBase::World *world = MWBase::Environment::get().getWorld();
            Ogre::Vector3 pos(ptr.getRefData().getPosition().pos);
            if(world->isUnderwater(ptr.getCell(), pos) || world->isWalkingOnWater(ptr))
                return 2;
            if(world->isOnGround(ptr))
                return 0;
            return -1;
        }
        if(name == "right")
        {
            MWBase::World *world = MWBase::Environment::get().getWorld();
            Ogre::Vector3 pos(ptr.getRefData().getPosition().pos);
            if(world->isUnderwater(ptr.getCell(), pos) || world->isWalkingOnWater(ptr))
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

    void Creature::readAdditionalState (const MWWorld::Ptr& ptr, const ESM::ObjectState& state)
        const
    {
        if (!state.mHasCustomState)
            return;

        const ESM::CreatureState& state2 = dynamic_cast<const ESM::CreatureState&> (state);

        ensureCustomData(ptr);

        // If we do the following instead we get a sizable speedup, but this causes compatibility issues
        // with 0.30 savegames, where some state in CreatureStats was not saved yet,
        // and therefore needs to be loaded from ESM records. TODO: re-enable this in a future release.
        /*
        if (!ptr.getRefData().getCustomData())
        {
            // Create a CustomData, but don't fill it from ESM records (not needed)
            std::auto_ptr<CreatureCustomData> data (new CreatureCustomData);

            MWWorld::LiveCellRef<ESM::Creature> *ref = ptr.get<ESM::Creature>();

            if (ref->mBase->mFlags & ESM::Creature::Weapon)
                data->mContainerStore = new MWWorld::InventoryStore();
            else
                data->mContainerStore = new MWWorld::ContainerStore();

            ptr.getRefData().setCustomData (data.release());
        }
        */

        CreatureCustomData& customData = dynamic_cast<CreatureCustomData&> (*ptr.getRefData().getCustomData());

        customData.mContainerStore->readState (state2.mInventory);
        customData.mCreatureStats.readState (state2.mCreatureStats);
    }

    void Creature::writeAdditionalState (const MWWorld::Ptr& ptr, ESM::ObjectState& state)
        const
    {
        ESM::CreatureState& state2 = dynamic_cast<ESM::CreatureState&> (state);

        if (!ptr.getRefData().getCustomData())
        {
            state.mHasCustomState = false;
            return;
        }

        ensureCustomData (ptr);

        CreatureCustomData& customData = dynamic_cast<CreatureCustomData&> (*ptr.getRefData().getCustomData());

        customData.mContainerStore->writeState (state2.mInventory);
        customData.mCreatureStats.writeState (state2.mCreatureStats);
    }

    int Creature::getBaseGold(const MWWorld::Ptr& ptr) const
    {
        return ptr.get<ESM::Creature>()->mBase->mData.mGold;
    }

    void Creature::respawn(const MWWorld::Ptr &ptr) const
    {
        if (ptr.get<ESM::Creature>()->mBase->mFlags & ESM::Creature::Respawn)
        {
            // Note we do not respawn moved references in the cell they were moved to. Instead they are respawned in the original cell.
            // This also means we cannot respawn dynamically placed references with no content file connection.
            if (ptr.getCellRef().hasContentFile())
            {
                if (ptr.getRefData().getCount() == 0)
                    ptr.getRefData().setCount(1);

                // Reset to original position
                ptr.getRefData().setPosition(ptr.getCellRef().getPosition());

                ptr.getRefData().setCustomData(NULL);
            }
        }
    }

    void Creature::restock(const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM::Creature> *ref = ptr.get<ESM::Creature>();
        const ESM::InventoryList& list = ref->mBase->mInventory;
        MWWorld::ContainerStore& store = getContainerStore(ptr);
        store.restock(list, ptr, ptr.getCellRef().getRefId());
    }

    int Creature::getBaseFightRating(const MWWorld::Ptr &ptr) const
    {
        MWWorld::LiveCellRef<ESM::Creature> *ref = ptr.get<ESM::Creature>();
        return ref->mBase->mAiData.mFight;
    }

    void Creature::adjustScale(const MWWorld::Ptr &ptr, float &scale) const
    {
        MWWorld::LiveCellRef<ESM::Creature> *ref = ptr.get<ESM::Creature>();
        scale *= ref->mBase->mScale;
    }
}

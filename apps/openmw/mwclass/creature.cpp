#include "creature.hpp"

#include <components/misc/rng.hpp>
#include <components/debug/debuglog.hpp>
#include <components/esm/loadcrea.hpp>
#include <components/esm/creaturestate.hpp>
#include <components/settings/settings.hpp>

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
#include "../mwphysics/physicssystem.hpp"
#include "../mwworld/cellstore.hpp"
#include "../mwworld/localscripts.hpp"

#include "../mwrender/renderinginterface.hpp"
#include "../mwrender/objects.hpp"

#include "../mwgui/tooltips.hpp"

#include "../mwworld/inventorystore.hpp"

#include "../mwmechanics/npcstats.hpp"
#include "../mwmechanics/combat.hpp"
#include "../mwmechanics/actorutil.hpp"

namespace
{
    bool isFlagBitSet(const MWWorld::ConstPtr &ptr, ESM::Creature::Flags bitMask)
    {
        return (ptr.get<ESM::Creature>()->mBase->mFlags & bitMask) != 0;
    }
}

namespace MWClass
{

    class CreatureCustomData : public MWWorld::CustomData
    {
    public:
        MWMechanics::CreatureStats mCreatureStats;
        MWWorld::ContainerStore* mContainerStore; // may be InventoryStore for some creatures
        MWMechanics::Movement mMovement;

        virtual MWWorld::CustomData *clone() const;

        virtual CreatureCustomData& asCreatureCustomData()
        {
            return *this;
        }
        virtual const CreatureCustomData& asCreatureCustomData() const
        {
            return *this;
        }

        CreatureCustomData() : mContainerStore(0) {}
        virtual ~CreatureCustomData() { delete mContainerStore; }
    };

    MWWorld::CustomData *CreatureCustomData::clone() const
    {
        CreatureCustomData* cloned = new CreatureCustomData (*this);
        cloned->mContainerStore = mContainerStore->clone();
        return cloned;
    }

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
            std::unique_ptr<CreatureCustomData> data (new CreatureCustomData);

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

            // Persistent actors with 0 health do not play death animation
            if (data->mCreatureStats.isDead())
                data->mCreatureStats.setDeathAnimationFinished(isPersistent(ptr));

            // spells
            for (std::vector<std::string>::const_iterator iter (ref->mBase->mSpells.mList.begin());
                iter!=ref->mBase->mSpells.mList.end(); ++iter)
            {
                if (const ESM::Spell* spell = MWBase::Environment::get().getWorld()->getStore().get<ESM::Spell>().search(*iter))
                    data->mCreatureStats.getSpells().add (spell);
                else /// \todo add option to make this a fatal error message pop-up, but default to warning for vanilla compatibility
                    Log(Debug::Warning) << "Warning: ignoring nonexistent spell '" << *iter << "' on creature '" << ref->mBase->mId << "'";
            }

            // inventory
            bool hasInventory = hasInventoryStore(ptr);
            if (hasInventory)
                data->mContainerStore = new MWWorld::InventoryStore();
            else
                data->mContainerStore = new MWWorld::ContainerStore();

            data->mCreatureStats.setGoldPool(ref->mBase->mData.mGold);

            data->mCreatureStats.setNeedRecalcDynamicStats(false);

            // store
            ptr.getRefData().setCustomData(data.release());

            getContainerStore(ptr).fill(ref->mBase->mInventory, ptr.getCellRef().getRefId());

            if (hasInventory)
                getInventoryStore(ptr).autoEquip(ptr);
        }
    }

    void Creature::insertObjectRendering (const MWWorld::Ptr& ptr, const std::string& model, MWRender::RenderingInterface& renderingInterface) const
    {
        MWRender::Objects& objects = renderingInterface.getObjects();
        objects.insertCreature(ptr, model, hasInventoryStore(ptr));
    }

    std::string Creature::getModel(const MWWorld::ConstPtr &ptr) const
    {
        const MWWorld::LiveCellRef<ESM::Creature> *ref = ptr.get<ESM::Creature>();

        const std::string &model = ref->mBase->mModel;
        if (!model.empty()) {
            return "meshes\\" + model;
        }
        return "";
    }

    void Creature::getModelsToPreload(const MWWorld::Ptr &ptr, std::vector<std::string> &models) const
    {
        std::string model = getModel(ptr);
        if (!model.empty())
            models.push_back(model);

        // FIXME: use const version of InventoryStore functions once they are available
        if (hasInventoryStore(ptr))
        {
            const MWWorld::InventoryStore& invStore = getInventoryStore(ptr);
            for (int slot = 0; slot < MWWorld::InventoryStore::Slots; ++slot)
            {
                MWWorld::ConstContainerStoreIterator equipped = invStore.getSlot(slot);
                if (equipped != invStore.end())
                {
                    model = equipped->getClass().getModel(*equipped);
                    if (!model.empty())
                        models.push_back(model);
                }
            }
        }
    }

    std::string Creature::getName (const MWWorld::ConstPtr& ptr) const
    {
        const MWWorld::LiveCellRef<ESM::Creature> *ref = ptr.get<ESM::Creature>();
        const std::string& name = ref->mBase->mName;

        return !name.empty() ? name : ref->mBase->mId;
    }

    MWMechanics::CreatureStats& Creature::getCreatureStats (const MWWorld::Ptr& ptr) const
    {
        ensureCustomData (ptr);

        return ptr.getRefData().getCustomData()->asCreatureCustomData().mCreatureStats;
    }


    void Creature::hit(const MWWorld::Ptr& ptr, float attackStrength, int type) const
    {
        MWWorld::LiveCellRef<ESM::Creature> *ref =
            ptr.get<ESM::Creature>();
        const MWWorld::Store<ESM::GameSetting> &gmst = MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>();
        MWMechanics::CreatureStats &stats = getCreatureStats(ptr);

        if (stats.getDrawState() != MWMechanics::DrawState_Weapon)
            return;

        // Get the weapon used (if hand-to-hand, weapon = inv.end())
        MWWorld::Ptr weapon;
        if (hasInventoryStore(ptr))
        {
            MWWorld::InventoryStore &inv = getInventoryStore(ptr);
            MWWorld::ContainerStoreIterator weaponslot = inv.getSlot(MWWorld::InventoryStore::Slot_CarriedRight);
            if (weaponslot != inv.end() && weaponslot->getTypeName() == typeid(ESM::Weapon).name())
                weapon = *weaponslot;
        }

        MWMechanics::applyFatigueLoss(ptr, weapon, attackStrength);

        float dist = gmst.find("fCombatDistance")->mValue.getFloat();
        if (!weapon.isEmpty())
            dist *= weapon.get<ESM::Weapon>()->mBase->mData.mReach;

        // For AI actors, get combat targets to use in the ray cast. Only those targets will return a positive hit result.
        std::vector<MWWorld::Ptr> targetActors;
        stats.getAiSequence().getCombatTargets(targetActors);

        std::pair<MWWorld::Ptr, osg::Vec3f> result = MWBase::Environment::get().getWorld()->getHitContact(ptr, dist, targetActors);
        if (result.first.isEmpty())
            return; // Didn't hit anything

        MWWorld::Ptr victim = result.first;

        if (!victim.getClass().isActor())
            return; // Can't hit non-actors

        osg::Vec3f hitPosition (result.second);

        float hitchance = MWMechanics::getHitChance(ptr, victim, ref->mBase->mData.mCombat);

        if(Misc::Rng::roll0to99() >= hitchance)
        {
            victim.getClass().onHit(victim, 0.0f, false, MWWorld::Ptr(), ptr, osg::Vec3f(), false);
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

        float damage = min + (max - min) * attackStrength;
        bool healthdmg = true;
        if (!weapon.isEmpty())
        {
            const unsigned char *attack = nullptr;
            if(type == ESM::Weapon::AT_Chop)
                attack = weapon.get<ESM::Weapon>()->mBase->mData.mChop;
            else if(type == ESM::Weapon::AT_Slash)
                attack = weapon.get<ESM::Weapon>()->mBase->mData.mSlash;
            else if(type == ESM::Weapon::AT_Thrust)
                attack = weapon.get<ESM::Weapon>()->mBase->mData.mThrust;
            if(attack)
            {
                damage = attack[0] + ((attack[1]-attack[0])*attackStrength);
                MWMechanics::adjustWeaponDamage(damage, weapon, ptr);
                MWMechanics::resistNormalWeapon(victim, ptr, weapon, damage);
                MWMechanics::reduceWeaponCondition(damage, true, weapon, ptr);
            }

            // Apply "On hit" enchanted weapons
            MWMechanics::applyOnStrikeEnchantment(ptr, victim, weapon, hitPosition);
        }
        else if (isBipedal(ptr))
        {
            MWMechanics::getHandToHandDamage(ptr, victim, damage, healthdmg, attackStrength);
        }

        MWMechanics::applyElementalShields(ptr, victim);

        if (MWMechanics::blockMeleeAttack(ptr, victim, weapon, damage, attackStrength))
            damage = 0;

        MWMechanics::diseaseContact(victim, ptr);

        victim.getClass().onHit(victim, damage, healthdmg, weapon, ptr, hitPosition, true);
    }

    void Creature::onHit(const MWWorld::Ptr &ptr, float damage, bool ishealth, const MWWorld::Ptr &object, const MWWorld::Ptr &attacker, const osg::Vec3f &hitPosition, bool successful) const
    {
        MWMechanics::CreatureStats& stats = getCreatureStats(ptr);

        // NOTE: 'object' and/or 'attacker' may be empty.        
        if (!attacker.isEmpty() && attacker.getClass().isActor() && !stats.getAiSequence().isInCombat(attacker))
            stats.setAttacked(true);

        // Self defense
        bool setOnPcHitMe = true; // Note OnPcHitMe is not set for friendly hits.
        
        // No retaliation for totally static creatures (they have no movement or attacks anyway)
        if (isMobile(ptr) && !attacker.isEmpty())
            setOnPcHitMe = MWBase::Environment::get().getMechanicsManager()->actorAttacked(ptr, attacker);

        // Attacker and target store each other as hitattemptactor if they have no one stored yet
        if (!attacker.isEmpty() && attacker.getClass().isActor())
        {
            MWMechanics::CreatureStats& statsAttacker = attacker.getClass().getCreatureStats(attacker);
            // First handle the attacked actor
            if ((stats.getHitAttemptActorId() == -1)
                && (statsAttacker.getAiSequence().isInCombat(ptr)
                    || attacker == MWMechanics::getPlayer()))
                stats.setHitAttemptActorId(statsAttacker.getActorId());

            // Next handle the attacking actor
            if ((statsAttacker.getHitAttemptActorId() == -1)
                && (statsAttacker.getAiSequence().isInCombat(ptr)
                    || attacker == MWMechanics::getPlayer()))
                statsAttacker.setHitAttemptActorId(stats.getActorId());
        }

        if (!object.isEmpty())
            stats.setLastHitAttemptObject(object.getCellRef().getRefId());

        if (setOnPcHitMe && !attacker.isEmpty() && attacker == MWMechanics::getPlayer())
        {
            const std::string &script = ptr.get<ESM::Creature>()->mBase->mScript;
            /* Set the OnPCHitMe script variable. The script is responsible for clearing it. */
            if(!script.empty())
                ptr.getRefData().getLocals().setVarByInt(script, "onpchitme", 1);
        }

        if (!successful)
        {
            // Missed
            if (!attacker.isEmpty() && attacker == MWMechanics::getPlayer())
                MWBase::Environment::get().getSoundManager()->playSound3D(ptr, "miss", 1.0f, 1.0f);
            return;
        }

        if (!object.isEmpty())
            stats.setLastHitObject(object.getCellRef().getRefId());

        if (damage < 0.001f)
            damage = 0;

        if (damage > 0.f)
        {
            if (!attacker.isEmpty())
            {
                // Check for knockdown
                float agilityTerm = stats.getAttribute(ESM::Attribute::Agility).getModified() * getGmst().fKnockDownMult->mValue.getFloat();
                float knockdownTerm = stats.getAttribute(ESM::Attribute::Agility).getModified()
                        * getGmst().iKnockDownOddsMult->mValue.getInteger() * 0.01f + getGmst().iKnockDownOddsBase->mValue.getInteger();
                if (ishealth && agilityTerm <= damage && knockdownTerm <= Misc::Rng::roll0to99())
                    stats.setKnockedDown(true);
                else
                    stats.setHitRecovery(true); // Is this supposed to always occur?
            }

            if(ishealth)
            {
                damage *= damage / (damage + getArmorRating(ptr));
                damage = std::max(1.f, damage);
                if (!attacker.isEmpty())
                {
                    damage = scaleDamage(damage, attacker, ptr);
                    MWBase::Environment::get().getWorld()->spawnBloodEffect(ptr, hitPosition);
                }

                MWBase::Environment::get().getSoundManager()->playSound3D(ptr, "Health Damage", 1.0f, 1.0f);

                MWMechanics::DynamicStat<float> health(stats.getHealth());
                health.setCurrent(health.getCurrent() - damage);
                stats.setHealth(health);
            }
            else
            {
                MWMechanics::DynamicStat<float> fatigue(stats.getFatigue());
                fatigue.setCurrent(fatigue.getCurrent() - damage, true);
                stats.setFatigue(fatigue);
            }
        }
    }

    std::shared_ptr<MWWorld::Action> Creature::activate (const MWWorld::Ptr& ptr,
        const MWWorld::Ptr& actor) const
    {
        if(actor.getClass().isNpc() && actor.getClass().getNpcStats(actor).isWerewolf())
        {
            const MWWorld::ESMStore &store = MWBase::Environment::get().getWorld()->getStore();
            const ESM::Sound *sound = store.get<ESM::Sound>().searchRandom("WolfCreature");

            std::shared_ptr<MWWorld::Action> action(new MWWorld::FailedAction("#{sWerewolfRefusal}"));
            if(sound) action->setSound(sound->mId);

            return action;
        }

        const MWMechanics::CreatureStats& stats = getCreatureStats(ptr);

        if(stats.isDead())
        {
            bool canLoot = Settings::Manager::getBool ("can loot during death animation", "Game");

            // by default user can loot friendly actors during death animation
            if (canLoot && !stats.getAiSequence().isInCombat())
                return std::shared_ptr<MWWorld::Action>(new MWWorld::ActionOpen(ptr));

            // otherwise wait until death animation
            if(stats.isDeathAnimationFinished())
                return std::shared_ptr<MWWorld::Action>(new MWWorld::ActionOpen(ptr));
        }
        else if (!stats.getAiSequence().isInCombat() && !stats.getKnockedDown())
            return std::shared_ptr<MWWorld::Action>(new MWWorld::ActionTalk(ptr));

        // Tribunal and some mod companions oddly enough must use open action as fallback
        if (!getScript(ptr).empty() && ptr.getRefData().getLocals().getIntVar(getScript(ptr), "companion"))
            return std::shared_ptr<MWWorld::Action>(new MWWorld::ActionOpen(ptr));

        return std::shared_ptr<MWWorld::Action>(new MWWorld::FailedAction(""));
    }

    MWWorld::ContainerStore& Creature::getContainerStore (const MWWorld::Ptr& ptr) const
    {
        ensureCustomData (ptr);

        return *ptr.getRefData().getCustomData()->asCreatureCustomData().mContainerStore;
    }

    MWWorld::InventoryStore& Creature::getInventoryStore(const MWWorld::Ptr &ptr) const
    {
        if (hasInventoryStore(ptr))
            return dynamic_cast<MWWorld::InventoryStore&>(getContainerStore(ptr));
        else
            throw std::runtime_error("this creature has no inventory store");
    }

    bool Creature::hasInventoryStore(const MWWorld::Ptr &ptr) const
    {
        return isFlagBitSet(ptr, ESM::Creature::Weapon);
    }

    std::string Creature::getScript (const MWWorld::ConstPtr& ptr) const
    {
        const MWWorld::LiveCellRef<ESM::Creature> *ref = ptr.get<ESM::Creature>();

        return ref->mBase->mScript;
    }

    bool Creature::isEssential (const MWWorld::ConstPtr& ptr) const
    {
        return isFlagBitSet(ptr, ESM::Creature::Essential);
    }

    void Creature::registerSelf()
    {
        std::shared_ptr<Class> instance (new Creature);

        registerClass (typeid (ESM::Creature).name(), instance);
    }

    float Creature::getSpeed(const MWWorld::Ptr &ptr) const
    {
        MWMechanics::CreatureStats& stats = getCreatureStats(ptr);
        if (stats.isParalyzed() || stats.getKnockedDown() || stats.isDead())
            return 0.f;

        const GMST& gmst = getGmst();

        float walkSpeed = gmst.fMinWalkSpeedCreature->mValue.getFloat() + 0.01f * stats.getAttribute(ESM::Attribute::Speed).getModified()
                * (gmst.fMaxWalkSpeedCreature->mValue.getFloat() - gmst.fMinWalkSpeedCreature->mValue.getFloat());

        const MWBase::World *world = MWBase::Environment::get().getWorld();
        const MWMechanics::MagicEffects &mageffects = stats.getMagicEffects();

        bool running = stats.getStance(MWMechanics::CreatureStats::Stance_Run);

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
            flySpeed = gmst.fMinFlySpeed->mValue.getFloat() + flySpeed*(gmst.fMaxFlySpeed->mValue.getFloat() - gmst.fMinFlySpeed->mValue.getFloat());
            const float normalizedEncumbrance = getNormalizedEncumbrance(ptr);
            flySpeed *= 1.0f - gmst.fEncumberedMoveEffect->mValue.getFloat() * normalizedEncumbrance;
            flySpeed = std::max(0.0f, flySpeed);
            moveSpeed = flySpeed;
        }
        else if(world->isSwimming(ptr))
        {
            float swimSpeed = walkSpeed;
            if(running)
                swimSpeed = runSpeed;
            swimSpeed *= 1.0f + 0.01f * mageffects.get(ESM::MagicEffect::SwiftSwim).getMagnitude();
            swimSpeed *= gmst.fSwimRunBase->mValue.getFloat() + 0.01f*getSkill(ptr, ESM::Skill::Athletics) *
                                                    gmst.fSwimRunAthleticsMult->mValue.getFloat();
            moveSpeed = swimSpeed;
        }
        else if(running)
            moveSpeed = runSpeed;
        else
            moveSpeed = walkSpeed;
        if(getMovementSettings(ptr).mPosition[0] != 0 && getMovementSettings(ptr).mPosition[1] == 0)
            moveSpeed *= 0.75f;

        moveSpeed *= ptr.getClass().getMovementSettings(ptr).mSpeedFactor;

        return moveSpeed;
    }

    MWMechanics::Movement& Creature::getMovementSettings (const MWWorld::Ptr& ptr) const
    {
        ensureCustomData (ptr);

        return ptr.getRefData().getCustomData()->asCreatureCustomData().mMovement;
    }

    bool Creature::hasToolTip(const MWWorld::ConstPtr& ptr) const
    {
        if (!ptr.getRefData().getCustomData() || MWBase::Environment::get().getWindowManager()->isGuiMode())
            return true;

        const CreatureCustomData& customData = ptr.getRefData().getCustomData()->asCreatureCustomData();

        if (customData.mCreatureStats.isDead() && customData.mCreatureStats.isDeathAnimationFinished())
            return true;

        return !customData.mCreatureStats.getAiSequence().isInCombat();
    }

    MWGui::ToolTipInfo Creature::getToolTipInfo (const MWWorld::ConstPtr& ptr, int count) const
    {
        const MWWorld::LiveCellRef<ESM::Creature> *ref = ptr.get<ESM::Creature>();

        MWGui::ToolTipInfo info;
        info.caption = MyGUI::TextIterator::toTagsString(getName(ptr));

        std::string text;
        if (MWBase::Environment::get().getWindowManager()->getFullHelp())
            text += MWGui::ToolTips::getMiscString(ref->mBase->mScript, "Script");
        info.text = text;

        return info;
    }

    float Creature::getArmorRating (const MWWorld::Ptr& ptr) const
    {
        // Equipment armor rating is deliberately ignored.
        return getCreatureStats(ptr).getMagicEffects().get(ESM::MagicEffect::Shield).getMagnitude();
    }

    float Creature::getCapacity (const MWWorld::Ptr& ptr) const
    {
        const MWMechanics::CreatureStats& stats = getCreatureStats (ptr);
        return static_cast<float>(stats.getAttribute(ESM::Attribute::Strength).getModified() * 5);
    }

    int Creature::getServices(const MWWorld::ConstPtr &actor) const
    {
        return actor.get<ESM::Creature>()->mBase->mAiData.mServices;
    }

    bool Creature::isPersistent(const MWWorld::ConstPtr &actor) const
    {
        const MWWorld::LiveCellRef<ESM::Creature>* ref = actor.get<ESM::Creature>();
        return ref->mBase->mPersistent;
    }

    std::string Creature::getSoundIdFromSndGen(const MWWorld::Ptr &ptr, const std::string &name) const
    {
        int type = getSndGenTypeFromName(ptr, name);
        if (type < 0)
            return std::string();

        std::vector<const ESM::SoundGenerator*> sounds;
        std::vector<const ESM::SoundGenerator*> fallbacksounds;

        MWWorld::LiveCellRef<ESM::Creature>* ref = ptr.get<ESM::Creature>();

        const std::string& ourId = (ref->mBase->mOriginal.empty()) ? ptr.getCellRef().getRefId() : ref->mBase->mOriginal;

        const MWWorld::ESMStore &store = MWBase::Environment::get().getWorld()->getStore();
        auto sound = store.get<ESM::SoundGenerator>().begin();
        while (sound != store.get<ESM::SoundGenerator>().end())
        {
            if (type == sound->mType && !sound->mCreature.empty() && Misc::StringUtils::ciEqual(ourId, sound->mCreature))
                sounds.push_back(&*sound);
            if (type == sound->mType && sound->mCreature.empty())
                fallbacksounds.push_back(&*sound);
            ++sound;
        }

        if (sounds.empty())
        {
            const std::string model = getModel(ptr);
            if (!model.empty())
            {
                for (const ESM::Creature &creature : store.get<ESM::Creature>())
                {
                    if (creature.mId != ourId && creature.mOriginal != ourId && !creature.mModel.empty()
                     && Misc::StringUtils::ciEqual(model, "meshes\\" + creature.mModel))
                    {
                        const std::string& fallbackId = !creature.mOriginal.empty() ? creature.mOriginal : creature.mId;
                        sound = store.get<ESM::SoundGenerator>().begin();
                        while (sound != store.get<ESM::SoundGenerator>().end())
                        {
                            if (type == sound->mType && !sound->mCreature.empty()
                             && Misc::StringUtils::ciEqual(fallbackId, sound->mCreature))
                                sounds.push_back(&*sound);
                            ++sound;
                        }
                        break;
                    }
                }
            }
        }

        if (!sounds.empty())
            return sounds[Misc::Rng::rollDice(sounds.size())]->mSound;
        if (!fallbacksounds.empty())
            return fallbacksounds[Misc::Rng::rollDice(fallbacksounds.size())]->mSound;

        return std::string();
    }

    MWWorld::Ptr Creature::copyToCellImpl(const MWWorld::ConstPtr &ptr, MWWorld::CellStore &cell) const
    {
        const MWWorld::LiveCellRef<ESM::Creature> *ref = ptr.get<ESM::Creature>();

        return MWWorld::Ptr(cell.insert(ref), &cell);
    }

    bool Creature::isBipedal(const MWWorld::ConstPtr &ptr) const
    {
        return isFlagBitSet(ptr, ESM::Creature::Bipedal);
    }

    bool Creature::canFly(const MWWorld::ConstPtr &ptr) const
    {
        return isFlagBitSet(ptr, ESM::Creature::Flies);
    }

    bool Creature::canSwim(const MWWorld::ConstPtr &ptr) const
    {
        return isFlagBitSet(ptr, static_cast<ESM::Creature::Flags>(ESM::Creature::Swims | ESM::Creature::Bipedal));
    }

    bool Creature::canWalk(const MWWorld::ConstPtr &ptr) const
    {
        return isFlagBitSet(ptr, static_cast<ESM::Creature::Flags>(ESM::Creature::Walks | ESM::Creature::Bipedal));
    }

    int Creature::getSndGenTypeFromName(const MWWorld::Ptr &ptr, const std::string &name)
    {
        if(name == "left")
        {
            MWBase::World *world = MWBase::Environment::get().getWorld();
            if(world->isFlying(ptr))
                return -1;
            osg::Vec3f pos(ptr.getRefData().getPosition().asVec3());
            if(world->isUnderwater(ptr.getCell(), pos) || world->isWalkingOnWater(ptr))
                return ESM::SoundGenerator::SwimLeft;
            if(world->isOnGround(ptr))
                return ESM::SoundGenerator::LeftFoot;
            return -1;
        }
        if(name == "right")
        {
            MWBase::World *world = MWBase::Environment::get().getWorld();
            if(world->isFlying(ptr))
                return -1;
            osg::Vec3f pos(ptr.getRefData().getPosition().asVec3());
            if(world->isUnderwater(ptr.getCell(), pos) || world->isWalkingOnWater(ptr))
                return ESM::SoundGenerator::SwimRight;
            if(world->isOnGround(ptr))
                return ESM::SoundGenerator::RightFoot;
            return -1;
        }
        if(name == "swimleft")
            return ESM::SoundGenerator::SwimLeft;
        if(name == "swimright")
            return ESM::SoundGenerator::SwimRight;
        if(name == "moan")
            return ESM::SoundGenerator::Moan;
        if(name == "roar")
            return ESM::SoundGenerator::Roar;
        if(name == "scream")
            return ESM::SoundGenerator::Scream;
        if(name == "land")
            return ESM::SoundGenerator::Land;

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

    int Creature::getBloodTexture(const MWWorld::ConstPtr &ptr) const
    {
        return ptr.get<ESM::Creature>()->mBase->mBloodType;
    }

    void Creature::readAdditionalState (const MWWorld::Ptr& ptr, const ESM::ObjectState& state)
        const
    {
        if (!state.mHasCustomState)
            return;

        if (state.mVersion > 0)
        {
            if (!ptr.getRefData().getCustomData())
            {
                // Create a CustomData, but don't fill it from ESM records (not needed)
                std::unique_ptr<CreatureCustomData> data (new CreatureCustomData);

                if (hasInventoryStore(ptr))
                    data->mContainerStore = new MWWorld::InventoryStore();
                else
                    data->mContainerStore = new MWWorld::ContainerStore();

                ptr.getRefData().setCustomData (data.release());
            }
        }
        else
            ensureCustomData(ptr); // in openmw 0.30 savegames not all state was saved yet, so need to load it regardless.

        CreatureCustomData& customData = ptr.getRefData().getCustomData()->asCreatureCustomData();
        const ESM::CreatureState& creatureState = state.asCreatureState();
        customData.mContainerStore->readState (creatureState.mInventory);
        customData.mCreatureStats.readState (creatureState.mCreatureStats);
    }

    void Creature::writeAdditionalState (const MWWorld::ConstPtr& ptr, ESM::ObjectState& state)
        const
    {
        if (!ptr.getRefData().getCustomData())
        {
            state.mHasCustomState = false;
            return;
        }

        const CreatureCustomData& customData = ptr.getRefData().getCustomData()->asCreatureCustomData();
        ESM::CreatureState& creatureState = state.asCreatureState();
        customData.mContainerStore->writeState (creatureState.mInventory);
        customData.mCreatureStats.writeState (creatureState.mCreatureStats);
    }

    int Creature::getBaseGold(const MWWorld::ConstPtr& ptr) const
    {
        return ptr.get<ESM::Creature>()->mBase->mData.mGold;
    }

    void Creature::respawn(const MWWorld::Ptr &ptr) const
    {
        const MWMechanics::CreatureStats& creatureStats = getCreatureStats(ptr);
        if (ptr.getRefData().getCount() > 0 && !creatureStats.isDead())
            return;

        if (!creatureStats.isDeathAnimationFinished())
            return;

        const MWWorld::Store<ESM::GameSetting>& gmst = MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>();
        static const float fCorpseRespawnDelay = gmst.find("fCorpseRespawnDelay")->mValue.getFloat();
        static const float fCorpseClearDelay = gmst.find("fCorpseClearDelay")->mValue.getFloat();

        float delay = ptr.getRefData().getCount() == 0 ? fCorpseClearDelay : std::min(fCorpseRespawnDelay, fCorpseClearDelay);

        if (isFlagBitSet(ptr, ESM::Creature::Respawn)
                && creatureStats.getTimeOfDeath() + delay <= MWBase::Environment::get().getWorld()->getTimeStamp())
        {
            if (ptr.getCellRef().hasContentFile())
            {
                if (ptr.getRefData().getCount() == 0)
                {
                    ptr.getRefData().setCount(1);
                    const std::string& script = getScript(ptr);
                    if(!script.empty())
                        MWBase::Environment::get().getWorld()->getLocalScripts().add(script, ptr);
                }

                MWBase::Environment::get().getWorld()->removeContainerScripts(ptr);
                ptr.getRefData().setCustomData(nullptr);

                // Reset to original position
                MWBase::Environment::get().getWorld()->moveObject(ptr, ptr.getCellRef().getPosition().pos[0],
                        ptr.getCellRef().getPosition().pos[1],
                        ptr.getCellRef().getPosition().pos[2]);
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

    int Creature::getBaseFightRating(const MWWorld::ConstPtr &ptr) const
    {
        const MWWorld::LiveCellRef<ESM::Creature> *ref = ptr.get<ESM::Creature>();
        return ref->mBase->mAiData.mFight;
    }

    void Creature::adjustScale(const MWWorld::ConstPtr &ptr, osg::Vec3f &scale, bool /* rendering */) const
    {
        const MWWorld::LiveCellRef<ESM::Creature> *ref = ptr.get<ESM::Creature>();
        scale *= ref->mBase->mScale;
    }
}

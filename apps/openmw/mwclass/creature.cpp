#include "creature.hpp"

#include <MyGUI_TextIterator.h>

#include <components/esm3/creaturestate.hpp>
#include <components/esm3/loadclas.hpp>
#include <components/esm3/loadcrea.hpp>
#include <components/esm3/loadsndg.hpp>
#include <components/esm3/loadsoun.hpp>
#include <components/misc/rng.hpp>
#include <components/settings/settings.hpp>

#include "../mwmechanics/actorutil.hpp"
#include "../mwmechanics/aisetting.hpp"
#include "../mwmechanics/combat.hpp"
#include "../mwmechanics/creaturecustomdataresetter.hpp"
#include "../mwmechanics/creaturestats.hpp"
#include "../mwmechanics/difficultyscaling.hpp"
#include "../mwmechanics/disease.hpp"
#include "../mwmechanics/inventory.hpp"
#include "../mwmechanics/magiceffects.hpp"
#include "../mwmechanics/movement.hpp"
#include "../mwmechanics/npcstats.hpp"
#include "../mwmechanics/setbaseaisetting.hpp"

#include "../mwbase/environment.hpp"
#include "../mwbase/mechanicsmanager.hpp"
#include "../mwbase/soundmanager.hpp"
#include "../mwbase/windowmanager.hpp"
#include "../mwbase/world.hpp"

#include "../mwworld/actionopen.hpp"
#include "../mwworld/actiontalk.hpp"
#include "../mwworld/cellstore.hpp"
#include "../mwworld/containerstore.hpp"
#include "../mwworld/customdata.hpp"
#include "../mwworld/esmstore.hpp"
#include "../mwworld/failedaction.hpp"
#include "../mwworld/inventorystore.hpp"
#include "../mwworld/localscripts.hpp"
#include "../mwworld/ptr.hpp"

#include "../mwrender/objects.hpp"
#include "../mwrender/renderinginterface.hpp"

#include "../mwgui/tooltips.hpp"
#include "../mwgui/ustring.hpp"

#include "classmodel.hpp"

namespace
{
    bool isFlagBitSet(const MWWorld::ConstPtr& ptr, ESM::Creature::Flags bitMask)
    {
        return (ptr.get<ESM::Creature>()->mBase->mFlags & bitMask) != 0;
    }
}

namespace MWClass
{

    class CreatureCustomData : public MWWorld::TypedCustomData<CreatureCustomData>
    {
    public:
        MWMechanics::CreatureStats mCreatureStats;
        std::unique_ptr<MWWorld::ContainerStore> mContainerStore; // may be InventoryStore for some creatures
        MWMechanics::Movement mMovement;

        CreatureCustomData() = default;
        CreatureCustomData(const CreatureCustomData& other);
        CreatureCustomData(CreatureCustomData&& other) = default;

        CreatureCustomData& asCreatureCustomData() override { return *this; }
        const CreatureCustomData& asCreatureCustomData() const override { return *this; }
    };

    CreatureCustomData::CreatureCustomData(const CreatureCustomData& other)
        : mCreatureStats(other.mCreatureStats)
        , mContainerStore(other.mContainerStore->clone())
        , mMovement(other.mMovement)
    {
    }

    Creature::Creature()
        : MWWorld::RegisteredClass<Creature, Actor>(ESM::Creature::sRecordId)
    {
    }

    const Creature::GMST& Creature::getGmst()
    {
        static const GMST staticGmst = [] {
            GMST gmst;

            const MWWorld::Store<ESM::GameSetting>& store
                = MWBase::Environment::get().getESMStore()->get<ESM::GameSetting>();

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

            return gmst;
        }();
        return staticGmst;
    }

    void Creature::ensureCustomData(const MWWorld::Ptr& ptr) const
    {
        if (!ptr.getRefData().getCustomData())
        {
            auto tempData = std::make_unique<CreatureCustomData>();
            CreatureCustomData* data = tempData.get();
            MWMechanics::CreatureCustomDataResetter resetter{ ptr };
            ptr.getRefData().setCustomData(std::move(tempData));

            MWWorld::LiveCellRef<ESM::Creature>* ref = ptr.get<ESM::Creature>();

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

            data->mCreatureStats.setAiSetting(MWMechanics::AiSetting::Hello, ref->mBase->mAiData.mHello);
            data->mCreatureStats.setAiSetting(MWMechanics::AiSetting::Fight, ref->mBase->mAiData.mFight);
            data->mCreatureStats.setAiSetting(MWMechanics::AiSetting::Flee, ref->mBase->mAiData.mFlee);
            data->mCreatureStats.setAiSetting(MWMechanics::AiSetting::Alarm, ref->mBase->mAiData.mAlarm);

            // Persistent actors with 0 health do not play death animation
            if (data->mCreatureStats.isDead())
                data->mCreatureStats.setDeathAnimationFinished(isPersistent(ptr));

            // spells
            bool spellsInitialised = data->mCreatureStats.getSpells().setSpells(ref->mBase->mId);
            if (!spellsInitialised)
                data->mCreatureStats.getSpells().addAllToInstance(ref->mBase->mSpells.mList);

            // inventory
            bool hasInventory = hasInventoryStore(ptr);
            if (hasInventory)
                data->mContainerStore = std::make_unique<MWWorld::InventoryStore>();
            else
                data->mContainerStore = std::make_unique<MWWorld::ContainerStore>();

            data->mCreatureStats.setGoldPool(ref->mBase->mData.mGold);

            resetter.mPtr = {};

            auto& prng = MWBase::Environment::get().getWorld()->getPrng();
            getContainerStore(ptr).fill(ref->mBase->mInventory, ptr.getCellRef().getRefId(), prng);

            if (hasInventory)
                getInventoryStore(ptr).autoEquip();
        }
    }

    void Creature::insertObjectRendering(
        const MWWorld::Ptr& ptr, const std::string& model, MWRender::RenderingInterface& renderingInterface) const
    {
        MWRender::Objects& objects = renderingInterface.getObjects();
        objects.insertCreature(ptr, model, hasInventoryStore(ptr));
    }

    std::string Creature::getModel(const MWWorld::ConstPtr& ptr) const
    {
        return getClassModel<ESM::Creature>(ptr);
    }

    void Creature::getModelsToPreload(const MWWorld::Ptr& ptr, std::vector<std::string>& models) const
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

    std::string_view Creature::getName(const MWWorld::ConstPtr& ptr) const
    {
        const MWWorld::LiveCellRef<ESM::Creature>* ref = ptr.get<ESM::Creature>();
        const std::string& name = ref->mBase->mName;

        return !name.empty() ? name : ref->mBase->mId.getRefIdString();
    }

    MWMechanics::CreatureStats& Creature::getCreatureStats(const MWWorld::Ptr& ptr) const
    {
        ensureCustomData(ptr);

        return ptr.getRefData().getCustomData()->asCreatureCustomData().mCreatureStats;
    }

    bool Creature::evaluateHit(const MWWorld::Ptr& ptr, MWWorld::Ptr& victim, osg::Vec3f& hitPosition) const
    {
        victim = MWWorld::Ptr();
        hitPosition = osg::Vec3f();

        // Get the weapon used (if hand-to-hand, weapon = inv.end())
        MWWorld::Ptr weapon;
        if (hasInventoryStore(ptr))
        {
            MWWorld::InventoryStore& inv = getInventoryStore(ptr);
            MWWorld::ContainerStoreIterator weaponslot = inv.getSlot(MWWorld::InventoryStore::Slot_CarriedRight);
            if (weaponslot != inv.end() && weaponslot->getType() == ESM::Weapon::sRecordId)
                weapon = *weaponslot;
        }

        MWBase::World* world = MWBase::Environment::get().getWorld();
        const MWWorld::Store<ESM::GameSetting>& store = world->getStore().get<ESM::GameSetting>();
        float dist = store.find("fCombatDistance")->mValue.getFloat();
        if (!weapon.isEmpty())
            dist *= weapon.get<ESM::Weapon>()->mBase->mData.mReach;

        // For AI actors, get combat targets to use in the ray cast. Only those targets will return a positive hit
        // result.
        std::vector<MWWorld::Ptr> targetActors;
        getCreatureStats(ptr).getAiSequence().getCombatTargets(targetActors);

        std::pair<MWWorld::Ptr, osg::Vec3f> result
            = MWBase::Environment::get().getWorld()->getHitContact(ptr, dist, targetActors);
        if (result.first.isEmpty()) // Didn't hit anything
            return true;

        const MWWorld::Class& othercls = result.first.getClass();
        if (!othercls.isActor()) // Can't hit non-actors
            return true;

        MWMechanics::CreatureStats& otherstats = othercls.getCreatureStats(result.first);
        if (otherstats.isDead()) // Can't hit dead actors
            return true;

        // Note that earlier we returned true in spite of an apparent failure to hit anything alive.
        // This is because hitting nothing is not a "miss" and should be handled as such character controller-side.
        victim = result.first;
        hitPosition = result.second;

        float hitchance = MWMechanics::getHitChance(ptr, victim, ptr.get<ESM::Creature>()->mBase->mData.mCombat);
        return Misc::Rng::roll0to99(world->getPrng()) < hitchance;
    }

    void Creature::hit(const MWWorld::Ptr& ptr, float attackStrength, int type, const MWWorld::Ptr& victim,
        const osg::Vec3f& hitPosition, bool success) const
    {
        MWMechanics::CreatureStats& stats = getCreatureStats(ptr);

        if (stats.getDrawState() != MWMechanics::DrawState::Weapon)
            return;

        MWWorld::Ptr weapon;
        if (hasInventoryStore(ptr))
        {
            MWWorld::InventoryStore& inv = getInventoryStore(ptr);
            MWWorld::ContainerStoreIterator weaponslot = inv.getSlot(MWWorld::InventoryStore::Slot_CarriedRight);
            if (weaponslot != inv.end() && weaponslot->getType() == ESM::Weapon::sRecordId)
                weapon = *weaponslot;
        }

        MWMechanics::applyFatigueLoss(ptr, weapon, attackStrength);

        if (victim.isEmpty())
            return; // Didn't hit anything

        const MWWorld::Class& othercls = victim.getClass();
        MWMechanics::CreatureStats& otherstats = othercls.getCreatureStats(victim);
        if (otherstats.isDead()) // Can't hit dead actors
            return;

        if (!success)
        {
            victim.getClass().onHit(victim, 0.0f, false, MWWorld::Ptr(), ptr, osg::Vec3f(), false);
            MWMechanics::reduceWeaponCondition(0.f, false, weapon, ptr);
            return;
        }

        MWWorld::LiveCellRef<ESM::Creature>* ref = ptr.get<ESM::Creature>();
        int min, max;
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
            const unsigned char* attack = nullptr;
            if (type == ESM::Weapon::AT_Chop)
                attack = weapon.get<ESM::Weapon>()->mBase->mData.mChop;
            else if (type == ESM::Weapon::AT_Slash)
                attack = weapon.get<ESM::Weapon>()->mBase->mData.mSlash;
            else if (type == ESM::Weapon::AT_Thrust)
                attack = weapon.get<ESM::Weapon>()->mBase->mData.mThrust;
            if (attack)
            {
                damage = attack[0] + ((attack[1] - attack[0]) * attackStrength);
                MWMechanics::adjustWeaponDamage(damage, weapon, ptr);
                MWMechanics::reduceWeaponCondition(damage, true, weapon, ptr);
                MWMechanics::resistNormalWeapon(victim, ptr, weapon, damage);
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
        {
            damage = 0;
            victim.getClass().block(victim);
        }

        MWMechanics::diseaseContact(victim, ptr);

        victim.getClass().onHit(victim, damage, healthdmg, weapon, ptr, hitPosition, true);
    }

    void Creature::onHit(const MWWorld::Ptr& ptr, float damage, bool ishealth, const MWWorld::Ptr& object,
        const MWWorld::Ptr& attacker, const osg::Vec3f& hitPosition, bool successful) const
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
                && (statsAttacker.getAiSequence().isInCombat(ptr) || attacker == MWMechanics::getPlayer()))
                stats.setHitAttemptActorId(statsAttacker.getActorId());

            // Next handle the attacking actor
            if ((statsAttacker.getHitAttemptActorId() == -1)
                && (statsAttacker.getAiSequence().isInCombat(ptr) || attacker == MWMechanics::getPlayer()))
                statsAttacker.setHitAttemptActorId(stats.getActorId());
        }

        if (!object.isEmpty())
            stats.setLastHitAttemptObject(object.getCellRef().getRefId());

        if (setOnPcHitMe && !attacker.isEmpty() && attacker == MWMechanics::getPlayer())
        {
            const ESM::RefId& script = ptr.get<ESM::Creature>()->mBase->mScript;
            /* Set the OnPCHitMe script variable. The script is responsible for clearing it. */
            if (!script.empty())
                ptr.getRefData().getLocals().setVarByInt(script, "onpchitme", 1);
        }

        if (!successful)
        {
            // Missed
            if (!attacker.isEmpty() && attacker == MWMechanics::getPlayer())
                MWBase::Environment::get().getSoundManager()->playSound3D(
                    ptr, ESM::RefId::stringRefId("miss"), 1.0f, 1.0f);
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
                float agilityTerm = stats.getAttribute(ESM::Attribute::Agility).getModified()
                    * getGmst().fKnockDownMult->mValue.getFloat();
                float knockdownTerm = stats.getAttribute(ESM::Attribute::Agility).getModified()
                        * getGmst().iKnockDownOddsMult->mValue.getInteger() * 0.01f
                    + getGmst().iKnockDownOddsBase->mValue.getInteger();
                auto& prng = MWBase::Environment::get().getWorld()->getPrng();
                if (ishealth && agilityTerm <= damage && knockdownTerm <= Misc::Rng::roll0to99(prng))
                    stats.setKnockedDown(true);
                else
                    stats.setHitRecovery(true); // Is this supposed to always occur?
            }

            if (ishealth)
            {
                damage *= damage / (damage + getArmorRating(ptr));
                damage = std::max(1.f, damage);
                if (!attacker.isEmpty())
                {
                    damage = scaleDamage(damage, attacker, ptr);
                    MWBase::Environment::get().getWorld()->spawnBloodEffect(ptr, hitPosition);
                }

                MWBase::Environment::get().getSoundManager()->playSound3D(
                    ptr, ESM::RefId::stringRefId("Health Damage"), 1.0f, 1.0f);

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

    std::unique_ptr<MWWorld::Action> Creature::activate(const MWWorld::Ptr& ptr, const MWWorld::Ptr& actor) const
    {
        if (actor.getClass().isNpc() && actor.getClass().getNpcStats(actor).isWerewolf())
        {
            const MWWorld::ESMStore& store = *MWBase::Environment::get().getESMStore();
            auto& prng = MWBase::Environment::get().getWorld()->getPrng();
            const ESM::Sound* sound = store.get<ESM::Sound>().searchRandom("WolfCreature", prng);

            std::unique_ptr<MWWorld::Action> action = std::make_unique<MWWorld::FailedAction>("#{sWerewolfRefusal}");
            if (sound)
                action->setSound(sound->mId);

            return action;
        }

        const MWMechanics::CreatureStats& stats = getCreatureStats(ptr);

        if (stats.isDead())
        {
            bool canLoot = Settings::Manager::getBool("can loot during death animation", "Game");

            // by default user can loot friendly actors during death animation
            if (canLoot && !stats.getAiSequence().isInCombat())
                return std::make_unique<MWWorld::ActionOpen>(ptr);

            // otherwise wait until death animation
            if (stats.isDeathAnimationFinished())
                return std::make_unique<MWWorld::ActionOpen>(ptr);
        }
        else if (!stats.getAiSequence().isInCombat() && !stats.getKnockedDown())
            return std::make_unique<MWWorld::ActionTalk>(ptr);

        // Tribunal and some mod companions oddly enough must use open action as fallback
        if (!getScript(ptr).empty() && ptr.getRefData().getLocals().getIntVar(getScript(ptr), "companion"))
            return std::make_unique<MWWorld::ActionOpen>(ptr);

        return std::make_unique<MWWorld::FailedAction>();
    }

    MWWorld::ContainerStore& Creature::getContainerStore(const MWWorld::Ptr& ptr) const
    {
        ensureCustomData(ptr);
        auto& store = *ptr.getRefData().getCustomData()->asCreatureCustomData().mContainerStore;
        if (hasInventoryStore(ptr))
            static_cast<MWWorld::InventoryStore&>(store).setActor(ptr);
        return store;
    }

    MWWorld::InventoryStore& Creature::getInventoryStore(const MWWorld::Ptr& ptr) const
    {
        if (hasInventoryStore(ptr))
            return static_cast<MWWorld::InventoryStore&>(getContainerStore(ptr));
        else
            throw std::runtime_error("this creature has no inventory store");
    }

    bool Creature::hasInventoryStore(const MWWorld::Ptr& ptr) const
    {
        return isFlagBitSet(ptr, ESM::Creature::Weapon);
    }

    ESM::RefId Creature::getScript(const MWWorld::ConstPtr& ptr) const
    {
        const MWWorld::LiveCellRef<ESM::Creature>* ref = ptr.get<ESM::Creature>();

        return ref->mBase->mScript;
    }

    bool Creature::isEssential(const MWWorld::ConstPtr& ptr) const
    {
        return isFlagBitSet(ptr, ESM::Creature::Essential);
    }

    float Creature::getMaxSpeed(const MWWorld::Ptr& ptr) const
    {
        const MWMechanics::CreatureStats& stats = getCreatureStats(ptr);

        if (stats.isParalyzed() || stats.getKnockedDown() || stats.isDead())
            return 0.f;

        const GMST& gmst = getGmst();

        const MWBase::World* world = MWBase::Environment::get().getWorld();
        const MWMechanics::MagicEffects& mageffects = stats.getMagicEffects();

        float moveSpeed;

        if (getEncumbrance(ptr) > getCapacity(ptr))
            moveSpeed = 0.0f;
        else if (canFly(ptr)
            || (mageffects.get(ESM::MagicEffect::Levitate).getMagnitude() > 0 && world->isLevitationEnabled()))
        {
            float flySpeed = 0.01f
                * (stats.getAttribute(ESM::Attribute::Speed).getModified()
                    + mageffects.get(ESM::MagicEffect::Levitate).getMagnitude());
            flySpeed = gmst.fMinFlySpeed->mValue.getFloat()
                + flySpeed * (gmst.fMaxFlySpeed->mValue.getFloat() - gmst.fMinFlySpeed->mValue.getFloat());
            const float normalizedEncumbrance = getNormalizedEncumbrance(ptr);
            flySpeed *= 1.0f - gmst.fEncumberedMoveEffect->mValue.getFloat() * normalizedEncumbrance;
            flySpeed = std::max(0.0f, flySpeed);
            moveSpeed = flySpeed;
        }
        else if (world->isSwimming(ptr))
            moveSpeed = getSwimSpeed(ptr);
        else
            moveSpeed = getWalkSpeed(ptr);

        return moveSpeed;
    }

    MWMechanics::Movement& Creature::getMovementSettings(const MWWorld::Ptr& ptr) const
    {
        ensureCustomData(ptr);

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

    MWGui::ToolTipInfo Creature::getToolTipInfo(const MWWorld::ConstPtr& ptr, int count) const
    {
        const MWWorld::LiveCellRef<ESM::Creature>* ref = ptr.get<ESM::Creature>();

        MWGui::ToolTipInfo info;
        std::string_view name = getName(ptr);
        info.caption = MyGUI::TextIterator::toTagsString(MWGui::toUString(name));

        std::string text;
        if (MWBase::Environment::get().getWindowManager()->getFullHelp())
            text += MWGui::ToolTips::getMiscString(ref->mBase->mScript.getRefIdString(), "Script");
        info.text = text;

        return info;
    }

    float Creature::getArmorRating(const MWWorld::Ptr& ptr) const
    {
        // Equipment armor rating is deliberately ignored.
        return getCreatureStats(ptr).getMagicEffects().get(ESM::MagicEffect::Shield).getMagnitude();
    }

    float Creature::getCapacity(const MWWorld::Ptr& ptr) const
    {
        const MWMechanics::CreatureStats& stats = getCreatureStats(ptr);
        return stats.getAttribute(ESM::Attribute::Strength).getModified() * 5;
    }

    int Creature::getServices(const MWWorld::ConstPtr& actor) const
    {
        return actor.get<ESM::Creature>()->mBase->mAiData.mServices;
    }

    bool Creature::isPersistent(const MWWorld::ConstPtr& actor) const
    {
        const MWWorld::LiveCellRef<ESM::Creature>* ref = actor.get<ESM::Creature>();
        return (ref->mBase->mRecordFlags & ESM::FLAG_Persistent) != 0;
    }

    ESM::RefId Creature::getSoundIdFromSndGen(const MWWorld::Ptr& ptr, std::string_view name) const
    {
        int type = getSndGenTypeFromName(ptr, name);
        if (type < 0)
            return ESM::RefId();

        std::vector<const ESM::SoundGenerator*> sounds;
        std::vector<const ESM::SoundGenerator*> fallbacksounds;

        MWWorld::LiveCellRef<ESM::Creature>* ref = ptr.get<ESM::Creature>();

        const ESM::RefId& ourId = (ref->mBase->mOriginal.empty()) ? ptr.getCellRef().getRefId() : ref->mBase->mOriginal;

        const MWWorld::ESMStore& store = *MWBase::Environment::get().getESMStore();
        auto sound = store.get<ESM::SoundGenerator>().begin();
        while (sound != store.get<ESM::SoundGenerator>().end())
        {
            if (type == sound->mType && !sound->mCreature.empty() && ourId == sound->mCreature)
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
                const VFS::Manager* const vfs = MWBase::Environment::get().getResourceSystem()->getVFS();
                for (const ESM::Creature& creature : store.get<ESM::Creature>())
                {
                    if (creature.mId != ourId && creature.mOriginal != ourId && !creature.mModel.empty()
                        && Misc::StringUtils::ciEqual(
                            model, Misc::ResourceHelpers::correctMeshPath(creature.mModel, vfs)))
                    {
                        const ESM::RefId& fallbackId = !creature.mOriginal.empty() ? creature.mOriginal : creature.mId;
                        sound = store.get<ESM::SoundGenerator>().begin();
                        while (sound != store.get<ESM::SoundGenerator>().end())
                        {
                            if (type == sound->mType && !sound->mCreature.empty() && fallbackId == sound->mCreature)
                                sounds.push_back(&*sound);
                            ++sound;
                        }
                        break;
                    }
                }
            }
        }

        auto& prng = MWBase::Environment::get().getWorld()->getPrng();
        if (!sounds.empty())
            return sounds[Misc::Rng::rollDice(sounds.size(), prng)]->mSound;
        if (!fallbacksounds.empty())
            return fallbacksounds[Misc::Rng::rollDice(fallbacksounds.size(), prng)]->mSound;

        return ESM::RefId();
    }

    MWWorld::Ptr Creature::copyToCellImpl(const MWWorld::ConstPtr& ptr, MWWorld::CellStore& cell) const
    {
        const MWWorld::LiveCellRef<ESM::Creature>* ref = ptr.get<ESM::Creature>();

        return MWWorld::Ptr(cell.insert(ref), &cell);
    }

    bool Creature::isBipedal(const MWWorld::ConstPtr& ptr) const
    {
        return isFlagBitSet(ptr, ESM::Creature::Bipedal);
    }

    bool Creature::canFly(const MWWorld::ConstPtr& ptr) const
    {
        return isFlagBitSet(ptr, ESM::Creature::Flies);
    }

    bool Creature::canSwim(const MWWorld::ConstPtr& ptr) const
    {
        return isFlagBitSet(ptr, static_cast<ESM::Creature::Flags>(ESM::Creature::Swims | ESM::Creature::Bipedal));
    }

    bool Creature::canWalk(const MWWorld::ConstPtr& ptr) const
    {
        return isFlagBitSet(ptr, static_cast<ESM::Creature::Flags>(ESM::Creature::Walks | ESM::Creature::Bipedal));
    }

    int Creature::getSndGenTypeFromName(const MWWorld::Ptr& ptr, std::string_view name)
    {
        if (name == "left")
        {
            MWBase::World* world = MWBase::Environment::get().getWorld();
            if (world->isFlying(ptr))
                return -1;
            osg::Vec3f pos(ptr.getRefData().getPosition().asVec3());
            if (world->isUnderwater(ptr.getCell(), pos) || world->isWalkingOnWater(ptr))
                return ESM::SoundGenerator::SwimLeft;
            if (world->isOnGround(ptr))
                return ESM::SoundGenerator::LeftFoot;
            return -1;
        }
        if (name == "right")
        {
            MWBase::World* world = MWBase::Environment::get().getWorld();
            if (world->isFlying(ptr))
                return -1;
            osg::Vec3f pos(ptr.getRefData().getPosition().asVec3());
            if (world->isUnderwater(ptr.getCell(), pos) || world->isWalkingOnWater(ptr))
                return ESM::SoundGenerator::SwimRight;
            if (world->isOnGround(ptr))
                return ESM::SoundGenerator::RightFoot;
            return -1;
        }
        if (name == "swimleft")
            return ESM::SoundGenerator::SwimLeft;
        if (name == "swimright")
            return ESM::SoundGenerator::SwimRight;
        if (name == "moan")
            return ESM::SoundGenerator::Moan;
        if (name == "roar")
            return ESM::SoundGenerator::Roar;
        if (name == "scream")
            return ESM::SoundGenerator::Scream;
        if (name == "land")
            return ESM::SoundGenerator::Land;

        throw std::runtime_error("Unexpected soundgen type: " + std::string(name));
    }

    float Creature::getSkill(const MWWorld::Ptr& ptr, int skill) const
    {
        MWWorld::LiveCellRef<ESM::Creature>* ref = ptr.get<ESM::Creature>();

        const ESM::Skill* skillRecord = MWBase::Environment::get().getESMStore()->get<ESM::Skill>().find(skill);

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

    int Creature::getBloodTexture(const MWWorld::ConstPtr& ptr) const
    {
        return ptr.get<ESM::Creature>()->mBase->mBloodType;
    }

    void Creature::readAdditionalState(const MWWorld::Ptr& ptr, const ESM::ObjectState& state) const
    {
        if (!state.mHasCustomState)
            return;

        const ESM::CreatureState& creatureState = state.asCreatureState();

        if (state.mVersion > 0)
        {
            if (!ptr.getRefData().getCustomData())
            {
                if (creatureState.mCreatureStats.mMissingACDT)
                    ensureCustomData(ptr);
                else
                {
                    // Create a CustomData, but don't fill it from ESM records (not needed)
                    auto data = std::make_unique<CreatureCustomData>();

                    if (hasInventoryStore(ptr))
                        data->mContainerStore = std::make_unique<MWWorld::InventoryStore>();
                    else
                        data->mContainerStore = std::make_unique<MWWorld::ContainerStore>();

                    ptr.getRefData().setCustomData(std::move(data));
                }
            }
        }
        else
            ensureCustomData(
                ptr); // in openmw 0.30 savegames not all state was saved yet, so need to load it regardless.

        CreatureCustomData& customData = ptr.getRefData().getCustomData()->asCreatureCustomData();

        customData.mContainerStore->readState(creatureState.mInventory);
        bool spellsInitialised = customData.mCreatureStats.getSpells().setSpells(ptr.get<ESM::Creature>()->mBase->mId);
        if (spellsInitialised)
            customData.mCreatureStats.getSpells().clear();
        customData.mCreatureStats.readState(creatureState.mCreatureStats);
    }

    void Creature::writeAdditionalState(const MWWorld::ConstPtr& ptr, ESM::ObjectState& state) const
    {
        if (!ptr.getRefData().getCustomData())
        {
            state.mHasCustomState = false;
            return;
        }

        const CreatureCustomData& customData = ptr.getRefData().getCustomData()->asCreatureCustomData();
        if (ptr.getRefData().getCount() <= 0
            && (!isFlagBitSet(ptr, ESM::Creature::Respawn) || !customData.mCreatureStats.isDead()))
        {
            state.mHasCustomState = false;
            return;
        }

        ESM::CreatureState& creatureState = state.asCreatureState();
        customData.mContainerStore->writeState(creatureState.mInventory);
        customData.mCreatureStats.writeState(creatureState.mCreatureStats);
    }

    int Creature::getBaseGold(const MWWorld::ConstPtr& ptr) const
    {
        return ptr.get<ESM::Creature>()->mBase->mData.mGold;
    }

    void Creature::respawn(const MWWorld::Ptr& ptr) const
    {
        const MWMechanics::CreatureStats& creatureStats = getCreatureStats(ptr);
        if (ptr.getRefData().getCount() > 0 && !creatureStats.isDead())
            return;

        if (!creatureStats.isDeathAnimationFinished())
            return;

        const MWWorld::Store<ESM::GameSetting>& gmst
            = MWBase::Environment::get().getESMStore()->get<ESM::GameSetting>();
        static const float fCorpseRespawnDelay = gmst.find("fCorpseRespawnDelay")->mValue.getFloat();
        static const float fCorpseClearDelay = gmst.find("fCorpseClearDelay")->mValue.getFloat();

        float delay
            = ptr.getRefData().getCount() == 0 ? fCorpseClearDelay : std::min(fCorpseRespawnDelay, fCorpseClearDelay);

        if (isFlagBitSet(ptr, ESM::Creature::Respawn)
            && creatureStats.getTimeOfDeath() + delay <= MWBase::Environment::get().getWorld()->getTimeStamp())
        {
            if (ptr.getCellRef().hasContentFile())
            {
                if (ptr.getRefData().getCount() == 0)
                {
                    ptr.getRefData().setCount(1);
                    const ESM::RefId& script = getScript(ptr);
                    if (!script.empty())
                        MWBase::Environment::get().getWorld()->getLocalScripts().add(script, ptr);
                }

                MWBase::Environment::get().getWorld()->removeContainerScripts(ptr);
                MWBase::Environment::get().getWindowManager()->onDeleteCustomData(ptr);
                ptr.getRefData().setCustomData(nullptr);

                // Reset to original position
                MWBase::Environment::get().getWorld()->moveObject(ptr, ptr.getCellRef().getPosition().asVec3());
                MWBase::Environment::get().getWorld()->rotateObject(
                    ptr, ptr.getCellRef().getPosition().asRotationVec3(), MWBase::RotationFlag_none);
            }
        }
    }

    int Creature::getBaseFightRating(const MWWorld::ConstPtr& ptr) const
    {
        const MWWorld::LiveCellRef<ESM::Creature>* ref = ptr.get<ESM::Creature>();
        return ref->mBase->mAiData.mFight;
    }

    void Creature::adjustScale(const MWWorld::ConstPtr& ptr, osg::Vec3f& scale, bool /* rendering */) const
    {
        const MWWorld::LiveCellRef<ESM::Creature>* ref = ptr.get<ESM::Creature>();
        scale *= ref->mBase->mScale;
    }

    void Creature::setBaseAISetting(const ESM::RefId& id, MWMechanics::AiSetting setting, int value) const
    {
        MWMechanics::setBaseAISetting<ESM::Creature>(id, setting, value);
    }

    void Creature::modifyBaseInventory(const ESM::RefId& actorId, const ESM::RefId& itemId, int amount) const
    {
        MWMechanics::modifyBaseInventory<ESM::Creature>(actorId, itemId, amount);
    }

    float Creature::getWalkSpeed(const MWWorld::Ptr& ptr) const
    {
        const MWMechanics::CreatureStats& stats = getCreatureStats(ptr);
        const GMST& gmst = getGmst();

        return gmst.fMinWalkSpeedCreature->mValue.getFloat()
            + 0.01f * stats.getAttribute(ESM::Attribute::Speed).getModified()
            * (gmst.fMaxWalkSpeedCreature->mValue.getFloat() - gmst.fMinWalkSpeedCreature->mValue.getFloat());
    }

    float Creature::getRunSpeed(const MWWorld::Ptr& ptr) const
    {
        return getWalkSpeed(ptr);
    }

    float Creature::getSwimSpeed(const MWWorld::Ptr& ptr) const
    {
        const MWMechanics::CreatureStats& stats = getCreatureStats(ptr);
        const MWMechanics::MagicEffects& mageffects = stats.getMagicEffects();

        return getSwimSpeedImpl(ptr, getGmst(), mageffects, getWalkSpeed(ptr));
    }
}

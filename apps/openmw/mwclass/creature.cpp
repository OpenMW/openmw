#include "creature.hpp"

#include <MyGUI_TextIterator.h>
#include <MyGUI_UString.h>

#include <components/esm3/creaturestate.hpp>
#include <components/esm3/loadclas.hpp>
#include <components/esm3/loadcrea.hpp>
#include <components/esm3/loadsndg.hpp>
#include <components/esm3/loadsoun.hpp>
#include <components/misc/rng.hpp>
#include <components/settings/values.hpp>

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
#include "../mwbase/luamanager.hpp"
#include "../mwbase/mechanicsmanager.hpp"
#include "../mwbase/soundmanager.hpp"
#include "../mwbase/windowmanager.hpp"
#include "../mwbase/world.hpp"

#include "../mwlua/localscripts.hpp"

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
#include "../mwworld/worldmodel.hpp"

#include "../mwrender/objects.hpp"
#include "../mwrender/renderinginterface.hpp"

#include "../mwgui/tooltips.hpp"

#include "classmodel.hpp"
#include "nameorid.hpp"

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
            MWBase::Environment::get().getWorldModel()->registerPtr(ptr);
            auto tempData = std::make_unique<CreatureCustomData>();
            CreatureCustomData* data = tempData.get();
            MWMechanics::CreatureCustomDataResetter resetter{ ptr };
            ptr.getRefData().setCustomData(std::move(tempData));

            MWWorld::LiveCellRef<ESM::Creature>* ref = ptr.get<ESM::Creature>();

            // creature stats
            for (size_t i = 0; i < ref->mBase->mData.mAttributes.size(); ++i)
                data->mCreatureStats.setAttribute(ESM::Attribute::indexToRefId(static_cast<int>(i)),
                    static_cast<float>(ref->mBase->mData.mAttributes[i]));
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
            data->mContainerStore->setPtr(ptr);

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

    std::string_view Creature::getModel(const MWWorld::ConstPtr& ptr) const
    {
        return getClassModel<ESM::Creature>(ptr);
    }

    void Creature::getModelsToPreload(const MWWorld::ConstPtr& ptr, std::vector<std::string_view>& models) const
    {
        std::string_view model = getModel(ptr);
        if (!model.empty())
            models.push_back(model);

        const MWWorld::CustomData* customData = ptr.getRefData().getCustomData();
        if (customData && hasInventoryStore(ptr))
        {
            const auto& invStore
                = static_cast<const MWWorld::InventoryStore&>(*customData->asCreatureCustomData().mContainerStore);
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
        return getNameOrId<ESM::Creature>(ptr);
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

        const float dist = MWMechanics::getMeleeWeaponReach(ptr, weapon);
        const std::pair<MWWorld::Ptr, osg::Vec3f> result = MWMechanics::getHitContact(ptr, dist);
        if (result.first.isEmpty()) // Didn't hit anything
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

        if (!MWMechanics::isInMeleeReach(ptr, victim, MWMechanics::getMeleeWeaponReach(ptr, weapon)))
            return;

        if (!success)
        {
            MWBase::Environment::get().getLuaManager()->onHit(ptr, victim, weapon, MWWorld::Ptr(), type, attackStrength,
                0.0f, false, hitPosition, false, MWMechanics::DamageSourceType::Melee);
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
                attack = weapon.get<ESM::Weapon>()->mBase->mData.mChop.data();
            else if (type == ESM::Weapon::AT_Slash)
                attack = weapon.get<ESM::Weapon>()->mBase->mData.mSlash.data();
            else if (type == ESM::Weapon::AT_Thrust)
                attack = weapon.get<ESM::Weapon>()->mBase->mData.mThrust.data();
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
            damage = 0;

        MWMechanics::diseaseContact(victim, ptr);

        MWBase::Environment::get().getLuaManager()->onHit(ptr, victim, weapon, MWWorld::Ptr(), type, attackStrength,
            damage, healthdmg, hitPosition, true, MWMechanics::DamageSourceType::Melee);
    }

    void Creature::onHit(const MWWorld::Ptr& ptr, const std::map<std::string, float>& damages, ESM::RefId object,
        const MWWorld::Ptr& attacker, bool successful, const MWMechanics::DamageSourceType sourceType) const
    {
        MWMechanics::CreatureStats& stats = getCreatureStats(ptr);

        // Self defense
        bool setOnPcHitMe = true;

        // NOTE: 'object' and/or 'attacker' may be empty.
        if (!attacker.isEmpty() && attacker.getClass().isActor() && !stats.getAiSequence().isInCombat(attacker))
        {
            stats.setAttacked(true);

            bool complain = sourceType == MWMechanics::DamageSourceType::Melee;
            bool supportFriendlyFire = sourceType != MWMechanics::DamageSourceType::Ranged;
            if (supportFriendlyFire && MWMechanics::friendlyHit(attacker, ptr, complain))
                setOnPcHitMe = false;
            else
                setOnPcHitMe = MWBase::Environment::get().getMechanicsManager()->actorAttacked(ptr, attacker);
        }

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

        if (!object.empty())
            stats.setLastHitAttemptObject(object);

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
            return;
        }

        if (!object.empty())
            stats.setLastHitObject(object);

        bool hasDamage = false;
        bool hasHealthDamage = false;
        float healthDamage = 0.f;
        for (auto& [stat, damage] : damages)
        {
            if (damage < 0.001f)
                continue;
            hasDamage = true;

            if (stat == "health")
            {
                hasHealthDamage = true;
                healthDamage = damage;
                MWMechanics::DynamicStat<float> health(getCreatureStats(ptr).getHealth());
                health.setCurrent(health.getCurrent() - damage);
                stats.setHealth(health);
            }
            else if (stat == "fatigue")
            {
                MWMechanics::DynamicStat<float> fatigue(getCreatureStats(ptr).getFatigue());
                fatigue.setCurrent(fatigue.getCurrent() - damage, true);
                stats.setFatigue(fatigue);
            }
            else if (stat == "magicka")
            {
                MWMechanics::DynamicStat<float> magicka(getCreatureStats(ptr).getMagicka());
                magicka.setCurrent(magicka.getCurrent() - damage);
                stats.setMagicka(magicka);
            }
        }

        if (hasDamage)
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
                if (hasHealthDamage && agilityTerm <= healthDamage && knockdownTerm <= Misc::Rng::roll0to99(prng))
                    stats.setKnockedDown(true);
                else
                    stats.setHitRecovery(true); // Is this supposed to always occur?
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
            // by default user can loot non-fighting actors during death animation
            if (Settings::game().mCanLootDuringDeathAnimation)
                return std::make_unique<MWWorld::ActionOpen>(ptr);

            // otherwise wait until death animation
            if (stats.isDeathAnimationFinished())
                return std::make_unique<MWWorld::ActionOpen>(ptr);
        }
        else if (!stats.getKnockedDown())
            return std::make_unique<MWWorld::ActionTalk>(ptr);

        // Tribunal and some mod companions oddly enough must use open action as fallback
        if (!getScript(ptr).empty() && ptr.getRefData().getLocals().getIntVar(getScript(ptr), "companion"))
            return std::make_unique<MWWorld::ActionOpen>(ptr);

        return std::make_unique<MWWorld::FailedAction>();
    }

    MWWorld::ContainerStore& Creature::getContainerStore(const MWWorld::Ptr& ptr) const
    {
        ensureCustomData(ptr);
        return *ptr.getRefData().getCustomData()->asCreatureCustomData().mContainerStore;
    }

    MWWorld::InventoryStore& Creature::getInventoryStore(const MWWorld::Ptr& ptr) const
    {
        if (hasInventoryStore(ptr))
            return static_cast<MWWorld::InventoryStore&>(getContainerStore(ptr));
        else
            throw std::runtime_error("this creature has no inventory store");
    }

    bool Creature::hasInventoryStore(const MWWorld::ConstPtr& ptr) const
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
        const float normalizedEncumbrance = getNormalizedEncumbrance(ptr);

        float moveSpeed;

        if (normalizedEncumbrance > 1.0f)
            moveSpeed = 0.0f;
        else if (canFly(ptr)
            || (mageffects.getOrDefault(ESM::MagicEffect::Levitate).getMagnitude() > 0 && world->isLevitationEnabled()))
        {
            float flySpeed = 0.01f
                * (stats.getAttribute(ESM::Attribute::Speed).getModified()
                    + mageffects.getOrDefault(ESM::MagicEffect::Levitate).getMagnitude());
            flySpeed = gmst.fMinFlySpeed->mValue.getFloat()
                + flySpeed * (gmst.fMaxFlySpeed->mValue.getFloat() - gmst.fMinFlySpeed->mValue.getFloat());
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

        const MWMechanics::AiSequence& aiSeq = customData.mCreatureStats.getAiSequence();
        return !aiSeq.isInCombat() || aiSeq.isFleeing();
    }

    MWGui::ToolTipInfo Creature::getToolTipInfo(const MWWorld::ConstPtr& ptr, int count) const
    {
        const MWWorld::LiveCellRef<ESM::Creature>* ref = ptr.get<ESM::Creature>();

        MWGui::ToolTipInfo info;
        std::string_view name = getName(ptr);
        info.caption = MyGUI::TextIterator::toTagsString(MyGUI::UString(name));

        if (MWBase::Environment::get().getWindowManager()->getFullHelp())
            info.extra += MWGui::ToolTips::getMiscString(ref->mBase->mScript.getRefIdString(), "Script");

        return info;
    }

    float Creature::getArmorRating(const MWWorld::Ptr& ptr, bool useLuaInterfaceIfAvailable) const
    {
        // Equipment armor rating is deliberately ignored.
        return getCreatureStats(ptr).getMagicEffects().getOrDefault(ESM::MagicEffect::Shield).getMagnitude();
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
            const std::string_view model = getModel(ptr);
            if (!model.empty())
            {
                for (const ESM::Creature& creature : store.get<ESM::Creature>())
                {
                    if (creature.mId != ourId && creature.mOriginal != ourId && !creature.mModel.empty()
                        && Misc::StringUtils::ciEqual(model, creature.mModel))
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
        MWWorld::Ptr newPtr(cell.insert(ref), &cell);
        if (newPtr.getRefData().getCustomData())
        {
            MWBase::Environment::get().getWorldModel()->registerPtr(newPtr);
            newPtr.getClass().getContainerStore(newPtr).setPtr(newPtr);
        }
        return newPtr;
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

    float Creature::getSkill(const MWWorld::Ptr& ptr, ESM::RefId id) const
    {
        MWWorld::LiveCellRef<ESM::Creature>* ref = ptr.get<ESM::Creature>();

        const ESM::Skill* skillRecord = MWBase::Environment::get().getESMStore()->get<ESM::Skill>().find(id);

        switch (skillRecord->mData.mSpecialization)
        {
            case ESM::Class::Combat:
                return static_cast<float>(ref->mBase->mData.mCombat);
            case ESM::Class::Magic:
                return static_cast<float>(ref->mBase->mData.mMagic);
            case ESM::Class::Stealth:
                return static_cast<float>(ref->mBase->mData.mStealth);
            default:
                throw std::runtime_error("invalid specialisation");
        }
    }

    void Creature::readAdditionalState(const MWWorld::Ptr& ptr, const ESM::ObjectState& state) const
    {
        if (!state.mHasCustomState)
            return;

        const ESM::CreatureState& creatureState = state.asCreatureState();

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

                MWBase::Environment::get().getWorldModel()->registerPtr(ptr);
                data->mContainerStore->setPtr(ptr);

                ptr.getRefData().setCustomData(std::move(data));
            }
        }

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
        if (ptr.getCellRef().getCount() <= 0
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
        if (ptr.getCellRef().getCount() > 0 && !creatureStats.isDead())
            return;

        if (!creatureStats.isDeathAnimationFinished())
            return;

        const MWWorld::Store<ESM::GameSetting>& gmst
            = MWBase::Environment::get().getESMStore()->get<ESM::GameSetting>();
        static const float fCorpseRespawnDelay = gmst.find("fCorpseRespawnDelay")->mValue.getFloat();
        static const float fCorpseClearDelay = gmst.find("fCorpseClearDelay")->mValue.getFloat();

        float delay
            = ptr.getCellRef().getCount() == 0 ? fCorpseClearDelay : std::min(fCorpseRespawnDelay, fCorpseClearDelay);

        if (isFlagBitSet(ptr, ESM::Creature::Respawn)
            && creatureStats.getTimeOfDeath() + delay <= MWBase::Environment::get().getWorld()->getTimeStamp())
        {
            if (ptr.getCellRef().hasContentFile())
            {
                if (ptr.getCellRef().getCount() == 0)
                {
                    ptr.getCellRef().setCount(1);
                    const ESM::RefId& script = getScript(ptr);
                    if (!script.empty())
                        MWBase::Environment::get().getWorld()->getLocalScripts().add(script, ptr);
                }

                MWBase::Environment::get().getWorld()->removeContainerScripts(ptr);
                MWBase::Environment::get().getWindowManager()->onDeleteCustomData(ptr);
                ptr.getRefData().setCustomData(nullptr);

                // Reset to original position
                MWBase::Environment::get().getWorld()->moveObject(
                    ptr, ptr.getCell()->getOriginCell(ptr), ptr.getCellRef().getPosition().asVec3());
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
        MWMechanics::setBaseAISetting<ESM::Creature>(id, setting, static_cast<unsigned char>(value));
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

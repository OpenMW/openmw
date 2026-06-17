#include "enchanting.hpp"

#include <components/esm3/loadcrea.hpp>
#include <components/esm3/loadmgef.hpp>
#include <components/misc/rng.hpp>
#include <components/settings/values.hpp>

#include "../mwworld/class.hpp"
#include "../mwworld/containerstore.hpp"
#include "../mwworld/esmstore.hpp"

#include "../mwbase/environment.hpp"
#include "../mwbase/mechanicsmanager.hpp"
#include "../mwbase/world.hpp"

#include "actorutil.hpp"
#include "creaturestats.hpp"
#include "spellutil.hpp"
#include "weapontype.hpp"

namespace MWMechanics
{
    Enchanting::Enchanting()
        : mCastStyle(ESM::Enchantment::CastOnce)
        , mSelfEnchanting(false)
        , mObjectType(0)
        , mWeaponType(-1)
    {
    }

    void Enchanting::setOldItem(const MWWorld::Ptr& oldItem)
    {
        mOldItemPtr = oldItem;
        mWeaponType = -1;
        mObjectType = 0;
        if (!itemEmpty())
        {
            mObjectType = mOldItemPtr.getType();
            if (mObjectType == ESM::Weapon::sRecordId)
                mWeaponType = mOldItemPtr.get<ESM::Weapon>()->mBase->mData.mType;
        }
    }

    void Enchanting::setNewItemName(const std::string& s)
    {
        mNewItemName = s;
    }

    void Enchanting::setEffect(const ESM::EffectList& effectList)
    {
        mEffectList = effectList;
    }

    int Enchanting::getCastStyle() const
    {
        return mCastStyle;
    }

    void Enchanting::setSoulGem(const MWWorld::Ptr& soulGem)
    {
        mSoulGemPtr = soulGem;
    }

    bool Enchanting::create()
    {
        const MWWorld::Ptr& player = getPlayer();
        MWWorld::ContainerStore& store = player.getClass().getContainerStore(player);
        ESM::Enchantment enchantment;
        enchantment.mData.mFlags = 0;
        enchantment.mData.mType = mCastStyle;
        enchantment.mData.mCost = getBaseCastCost();
        enchantment.mRecordFlags = 0;

        store.remove(mSoulGemPtr, 1);

        // Exception for Azura Star, new one will be added after enchanting
        auto azurasStarId = ESM::RefId::stringRefId("Misc_SoulGem_Azura");
        if (mSoulGemPtr.get<ESM::Miscellaneous>()->mBase->mId == azurasStarId)
            store.add(azurasStarId, 1);

        if (mSelfEnchanting)
        {
            auto& prng = MWBase::Environment::get().getWorld()->getPrng();
            if (getEnchantChance() <= (Misc::Rng::roll0to99(prng)))
                return false;

            mEnchanter.getClass().skillUsageSucceeded(
                mEnchanter, ESM::Skill::Enchant, ESM::Skill::Enchant_CreateMagicItem);
        }

        enchantment.mEffects = mEffectList;

        int count = getEnchantItemsCount();

        if (mCastStyle == ESM::Enchantment::ConstantEffect)
            enchantment.mData.mCharge = 0;
        else
            enchantment.mData.mCharge = getGemCharge() / count;

        // Try to find a dynamic enchantment with the same stats, create a new one if not found.
        const ESM::Enchantment* enchantmentPtr = getRecord(enchantment);
        if (enchantmentPtr == nullptr)
            enchantmentPtr = MWBase::Environment::get().getESMStore()->insert(enchantment);

        // Apply the enchantment
        const ESM::RefId& newItemId
            = mOldItemPtr.getClass().applyEnchantment(mOldItemPtr, enchantmentPtr->mId, getGemCharge(), mNewItemName);

        if (!mSelfEnchanting)
            payForEnchantment(count);

        // Add the new item to player inventory and remove the old one
        store.remove(mOldItemPtr, count);
        store.add(newItemId, count);

        return true;
    }

    void Enchanting::nextCastStyle()
    {
        if (itemEmpty())
            return;

        const bool powerfulSoul = getGemCharge() >= MWBase::Environment::get()
                                                        .getESMStore()
                                                        ->get<ESM::GameSetting>()
                                                        .find("iSoulAmountForConstantEffect")
                                                        ->mValue.getInteger();
        if ((mObjectType == ESM::Armor::sRecordId) || (mObjectType == ESM::Clothing::sRecordId))
        { // Armor or Clothing
            switch (mCastStyle)
            {
                case ESM::Enchantment::WhenUsed:
                    if (powerfulSoul)
                        mCastStyle = ESM::Enchantment::ConstantEffect;
                    return;
                default: // takes care of Constant effect too
                    mCastStyle = ESM::Enchantment::WhenUsed;
                    return;
            }
        }
        else if (mWeaponType != -1)
        { // Weapon
            ESM::WeaponType::Class weapclass = MWMechanics::getWeaponType(mWeaponType)->mWeaponClass;
            switch (mCastStyle)
            {
                case ESM::Enchantment::WhenStrikes:
                    if (weapclass == ESM::WeaponType::Melee || weapclass == ESM::WeaponType::Ranged)
                        mCastStyle = ESM::Enchantment::WhenUsed;
                    return;
                case ESM::Enchantment::WhenUsed:
                    if (powerfulSoul && weapclass != ESM::WeaponType::Ammo && weapclass != ESM::WeaponType::Thrown)
                        mCastStyle = ESM::Enchantment::ConstantEffect;
                    else if (weapclass != ESM::WeaponType::Ranged)
                        mCastStyle = ESM::Enchantment::WhenStrikes;
                    return;
                default: // takes care of Constant effect too
                    mCastStyle = ESM::Enchantment::WhenUsed;
                    if (weapclass != ESM::WeaponType::Ranged)
                        mCastStyle = ESM::Enchantment::WhenStrikes;
                    return;
            }
        }
        else if (mObjectType == ESM::Book::sRecordId)
        { // Scroll or Book
            mCastStyle = ESM::Enchantment::CastOnce;
            return;
        }

        // Fail case
        mCastStyle = ESM::Enchantment::CastOnce;
    }

    /*
     * Vanilla enchant cost formula:
     *
     *  Touch/Self:          (min + max) * baseCost * 0.025 * duration + area * baseCost * 0.025
     *  Target:       1.5 * ((min + max) * baseCost * 0.025 * duration + area * baseCost * 0.025)
     *  Constant eff:        (min + max) * baseCost * 2.5              + area * baseCost * 0.025
     *
     *  For multiple effects - cost of each effect is multiplied by number of effects that follows +1.
     *
     *  Note: Minimal value inside formula for 'min' and 'max' is 1. So in vanilla:
     *        (0 + 0) == (1 + 0) == (1 + 1) => 2 or (2 + 0) == (1 + 2) => 3
     *
     *  Formula on UESPWiki is not entirely correct.
     */
    std::vector<float> Enchanting::getEffectCosts() const
    {
        std::vector<float> costs;
        if (mEffectList.mList.empty())
            return costs;

        costs.reserve(mEffectList.mList.size());
        const MWWorld::ESMStore& store = *MWBase::Environment::get().getESMStore();
        const float fEffectCostMult = store.get<ESM::GameSetting>().find("fEffectCostMult")->mValue.getFloat();
        const float fEnchantmentConstantDurationMult
            = store.get<ESM::GameSetting>().find("fEnchantmentConstantDurationMult")->mValue.getFloat();

        float cost = 0.f;
        for (const ESM::IndexedENAMstruct& effect : mEffectList.mList)
        {
            float baseCost = (store.get<ESM::MagicEffect>().find(effect.mData.mEffectID))->mData.mBaseCost;
            int magMin = std::max(1, effect.mData.mMagnMin);
            int magMax = std::max(1, effect.mData.mMagnMax);
            int area = std::max(1, effect.mData.mArea);
            float duration = static_cast<float>(effect.mData.mDuration);
            if (mCastStyle == ESM::Enchantment::ConstantEffect)
                duration = fEnchantmentConstantDurationMult;

            cost += ((magMin + magMax) * duration + area) * baseCost * fEffectCostMult * 0.05f;

            cost = std::max(1.f, cost);

            if (effect.mData.mRange == ESM::RT_Target)
                cost *= 1.5f;

            costs.push_back(cost);
        }

        return costs;
    }

    float Enchanting::getEnchantPoints(bool precise) const
    {
        float enchantmentCost = 0.f;
        for (float cost : getEffectCosts())
            enchantmentCost += precise ? cost : std::floor(cost);

        return enchantmentCost;
    }

    const ESM::Enchantment* Enchanting::getRecord(const ESM::Enchantment& toFind) const
    {
        const MWWorld::Store<ESM::Enchantment>& enchantments
            = MWBase::Environment::get().getESMStore()->get<ESM::Enchantment>();
        MWWorld::Store<ESM::Enchantment>::iterator iter(enchantments.begin());
        iter += (enchantments.getSize() - enchantments.getDynamicSize());
        for (; iter != enchantments.end(); ++iter)
        {
            if (iter->mEffects.mList.size() != toFind.mEffects.mList.size())
                continue;

            if (iter->mData.mFlags != toFind.mData.mFlags || iter->mData.mType != toFind.mData.mType
                || iter->mData.mCost != toFind.mData.mCost || iter->mData.mCharge != toFind.mData.mCharge)
                continue;

            // Don't choose an ID that came from the content files, would have unintended side effects
            if (!enchantments.isDynamic(iter->mId))
                continue;

            bool mismatch = false;

            for (int i = 0; i < static_cast<int>(iter->mEffects.mList.size()); ++i)
            {
                if (iter->mEffects.mList[i] != toFind.mEffects.mList[i])
                {
                    mismatch = true;
                    break;
                }
            }

            if (!mismatch)
                return &(*iter);
        }

        return nullptr;
    }

    int Enchanting::getBaseCastCost() const
    {
        if (mCastStyle == ESM::Enchantment::ConstantEffect)
            return 0;

        return static_cast<int>(getEnchantPoints(false));
    }

    int Enchanting::getEffectiveCastCost() const
    {
        int baseCost = getBaseCastCost();
        MWWorld::Ptr player = getPlayer();
        return getEffectiveEnchantmentCastCost(static_cast<float>(baseCost), player);
    }

    int Enchanting::getEnchantPrice(int count) const
    {
        if (mEnchanter.isEmpty())
            return 0;

        // Use the final effect's accumulated cost
        float finalEffectCost = 0.f;
        std::vector<float> effectCosts = getEffectCosts();
        if (!effectCosts.empty())
            finalEffectCost = effectCosts.back();

        float priceMultipler = MWBase::Environment::get()
                                   .getESMStore()
                                   ->get<ESM::GameSetting>()
                                   .find("fEnchantmentValueMult")
                                   ->mValue.getFloat();
        int price = MWBase::Environment::get().getMechanicsManager()->getBarterOffer(
            mEnchanter, static_cast<int>(finalEffectCost * priceMultipler), true);
        price *= static_cast<int>(count * getTypeMultiplier());
        return std::max(1, price);
    }

    int Enchanting::getGemCharge() const
    {
        const MWWorld::ESMStore& store = *MWBase::Environment::get().getESMStore();
        if (soulEmpty())
            return 0;
        if (mSoulGemPtr.getCellRef().getSoul().empty())
            return 0;
        const ESM::Creature* soul = store.get<ESM::Creature>().search(mSoulGemPtr.getCellRef().getSoul());
        if (soul)
            return soul->mData.mSoul;
        else
            return 0;
    }

    int Enchanting::getMaxEnchantValue() const
    {
        if (itemEmpty())
            return 0;

        const MWWorld::ESMStore& store = *MWBase::Environment::get().getESMStore();

        return static_cast<int>(mOldItemPtr.getClass().getEnchantmentPoints(mOldItemPtr)
            * store.get<ESM::GameSetting>().find("fEnchantmentMult")->mValue.getFloat());
    }
    bool Enchanting::soulEmpty() const
    {
        return mSoulGemPtr.isEmpty();
    }

    bool Enchanting::itemEmpty() const
    {
        return mOldItemPtr.isEmpty();
    }

    void Enchanting::setSelfEnchanting(bool selfEnchanting)
    {
        mSelfEnchanting = selfEnchanting;
    }

    void Enchanting::setEnchanter(const MWWorld::Ptr& enchanter)
    {
        mEnchanter = enchanter;
        // Reset cast style
        mCastStyle = ESM::Enchantment::CastOnce;
    }

    int Enchanting::getEnchantChance() const
    {
        const CreatureStats& stats = mEnchanter.getClass().getCreatureStats(mEnchanter);
        const MWWorld::Store<ESM::GameSetting>& gmst
            = MWBase::Environment::get().getESMStore()->get<ESM::GameSetting>();
        const float a = static_cast<float>(mEnchanter.getClass().getSkill(mEnchanter, ESM::Skill::Enchant));
        const float b = static_cast<float>(stats.getAttribute(ESM::Attribute::Intelligence).getModified());
        const float c = static_cast<float>(stats.getAttribute(ESM::Attribute::Luck).getModified());
        const float fEnchantmentChanceMult = gmst.find("fEnchantmentChanceMult")->mValue.getFloat();
        const float fEnchantmentConstantChanceMult = gmst.find("fEnchantmentConstantChanceMult")->mValue.getFloat();

        float x = (a - getEnchantPoints() * fEnchantmentChanceMult * getTypeMultiplier() * getEnchantItemsCount()
                      + 0.2f * b + 0.1f * c)
            * stats.getFatigueTerm();
        if (mCastStyle == ESM::Enchantment::ConstantEffect)
            x *= fEnchantmentConstantChanceMult;

        return static_cast<int>(x);
    }

    int Enchanting::getEnchantItemsCount() const
    {
        int count = 1;
        float enchantPoints = getEnchantPoints();
        if (mWeaponType != -1 && enchantPoints > 0)
        {
            ESM::WeaponType::Class weapclass = MWMechanics::getWeaponType(mWeaponType)->mWeaponClass;
            if (weapclass == ESM::WeaponType::Thrown || weapclass == ESM::WeaponType::Ammo)
            {
                MWWorld::Ptr player = getPlayer();
                count = player.getClass().getContainerStore(player).count(mOldItemPtr.getCellRef().getRefId());
                count = std::clamp(
                    static_cast<int>(getGemCharge() * Settings::game().mProjectilesEnchantMultiplier / enchantPoints),
                    1, count);
            }
        }

        return count;
    }

    float Enchanting::getTypeMultiplier() const
    {
        if (Settings::game().mProjectilesEnchantMultiplier > 0 && mWeaponType != -1 && getEnchantPoints() > 0)
        {
            ESM::WeaponType::Class weapclass = MWMechanics::getWeaponType(mWeaponType)->mWeaponClass;
            if (weapclass == ESM::WeaponType::Thrown || weapclass == ESM::WeaponType::Ammo)
                return 0.125f;
        }

        return 1.f;
    }

    void Enchanting::payForEnchantment(int count) const
    {
        const MWWorld::Ptr& player = getPlayer();
        MWWorld::ContainerStore& store = player.getClass().getContainerStore(player);

        int price = getEnchantPrice(count);
        store.remove(MWWorld::ContainerStore::sGoldId, price);

        // add gold to NPC trading gold pool
        CreatureStats& enchanterStats = mEnchanter.getClass().getCreatureStats(mEnchanter);
        enchanterStats.setGoldPool(enchanterStats.getGoldPool() + price);
    }
}

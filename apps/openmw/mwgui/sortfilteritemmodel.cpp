#include "sortfilteritemmodel.hpp"

#include <components/debug/debuglog.hpp>
#include <components/esm3/loadalch.hpp>
#include <components/esm3/loadappa.hpp>
#include <components/esm3/loadarmo.hpp>
#include <components/esm3/loadbook.hpp>
#include <components/esm3/loadclot.hpp>
#include <components/esm3/loadcrea.hpp>
#include <components/esm3/loadench.hpp>
#include <components/esm3/loadingr.hpp>
#include <components/esm3/loadligh.hpp>
#include <components/esm3/loadlock.hpp>
#include <components/esm3/loadmisc.hpp>
#include <components/esm3/loadprob.hpp>
#include <components/esm3/loadrepa.hpp>
#include <components/esm3/loadweap.hpp>
#include <components/misc/utf8stream.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"

#include "../mwworld/action.hpp"
#include "../mwworld/class.hpp"
#include "../mwworld/esmstore.hpp"

#include "../mwmechanics/alchemy.hpp"
#include "../mwmechanics/spellutil.hpp"

namespace
{
    unsigned int getTypeOrder(unsigned int type)
    {
        switch (type)
        {
            case ESM::Weapon::sRecordId:
                return 0;
            case ESM::Armor::sRecordId:
                return 1;
            case ESM::Clothing::sRecordId:
                return 2;
            case ESM::Potion::sRecordId:
                return 3;
            case ESM::Ingredient::sRecordId:
                return 4;
            case ESM::Apparatus::sRecordId:
                return 5;
            case ESM::Book::sRecordId:
                return 6;
            case ESM::Light::sRecordId:
                return 7;
            case ESM::Miscellaneous::sRecordId:
                return 8;
            case ESM::Lockpick::sRecordId:
                return 9;
            case ESM::Repair::sRecordId:
                return 10;
            case ESM::Probe::sRecordId:
                return 11;
        }
        assert(false && "Invalid type value");
        return std::numeric_limits<unsigned int>::max();
    }

    bool compareType(unsigned int type1, unsigned int type2)
    {
        return getTypeOrder(type1) < getTypeOrder(type2);
    }

    struct Compare
    {
        bool mSortByType;
        Compare()
            : mSortByType(true)
        {
        }
        bool operator()(const MWGui::ItemStack& left, const MWGui::ItemStack& right)
        {
            if (mSortByType && left.mType != right.mType)
                return left.mType < right.mType;

            // compare items by type
            auto leftType = left.mBase.getType();
            auto rightType = right.mBase.getType();

            if (leftType != rightType)
                return compareType(leftType, rightType);

            // compare items by name
            std::string leftName = Utf8Stream::lowerCaseUtf8(left.mBase.getClass().getName(left.mBase));
            std::string rightName = Utf8Stream::lowerCaseUtf8(right.mBase.getClass().getName(right.mBase));

            {
                int result = leftName.compare(rightName);
                if (result != 0)
                    return result < 0;
            }

            // compare items by enchantment:
            // 1. enchanted items showed before non-enchanted
            // 2. item with lesser charge percent comes after items with more charge percent
            // 3. item with constant effect comes before items with non-constant effects
            int leftChargePercent = -1;
            int rightChargePercent = -1;
            const ESM::RefId& leftNameEnch = left.mBase.getClass().getEnchantment(left.mBase);
            const ESM::RefId& rightNameEnch = right.mBase.getClass().getEnchantment(right.mBase);

            if (!leftNameEnch.empty())
            {
                const ESM::Enchantment* ench
                    = MWBase::Environment::get().getESMStore()->get<ESM::Enchantment>().search(leftNameEnch);
                if (ench)
                {
                    if (ench->mData.mType == ESM::Enchantment::ConstantEffect)
                        leftChargePercent = 101;
                    else
                        leftChargePercent
                            = static_cast<int>(left.mBase.getCellRef().getNormalizedEnchantmentCharge(*ench) * 100);
                }
            }

            if (!rightNameEnch.empty())
            {
                const ESM::Enchantment* ench
                    = MWBase::Environment::get().getESMStore()->get<ESM::Enchantment>().search(rightNameEnch);
                if (ench)
                {
                    if (ench->mData.mType == ESM::Enchantment::ConstantEffect)
                        rightChargePercent = 101;
                    else
                        rightChargePercent
                            = static_cast<int>(right.mBase.getCellRef().getNormalizedEnchantmentCharge(*ench) * 100);
                }
            }

            {
                int result = leftChargePercent - rightChargePercent;
                if (result != 0)
                    return result > 0;
            }

            // compare items by condition
            if (left.mBase.getClass().hasItemHealth(left.mBase) && right.mBase.getClass().hasItemHealth(right.mBase))
            {
                int result = left.mBase.getClass().getItemHealth(left.mBase)
                    - right.mBase.getClass().getItemHealth(right.mBase);
                if (result != 0)
                    return result > 0;
            }

            // compare items by remaining usage time
            {
                float result = left.mBase.getClass().getRemainingUsageTime(left.mBase)
                    - right.mBase.getClass().getRemainingUsageTime(right.mBase);
                if (result != 0)
                    return result > 0;
            }

            // compare items by value
            {
                int result = left.mBase.getClass().getValue(left.mBase) - right.mBase.getClass().getValue(right.mBase);
                if (result != 0)
                    return result > 0;
            }

            // compare items by weight
            float result = left.mBase.getClass().getWeight(left.mBase) - right.mBase.getClass().getWeight(right.mBase);
            if (result != 0)
                return result > 0;

            return left.mBase.getCellRef().getRefId() < right.mBase.getCellRef().getRefId();
        }
    };
}

namespace MWGui
{

    SortFilterItemModel::SortFilterItemModel(std::unique_ptr<ItemModel> sourceModel)
        : mCategory(Category_All)
        , mFilter(0)
        , mSortByType(true)
        , mApparatusTypeFilter(-1)
    {
        mSourceModel = std::move(sourceModel);
    }

    bool SortFilterItemModel::allowedToUseItems() const
    {
        return mSourceModel->allowedToUseItems();
    }

    void SortFilterItemModel::addDragItem(const MWWorld::Ptr& dragItem, size_t count)
    {
        mDragItems.emplace_back(dragItem, count);
    }

    void SortFilterItemModel::clearDragItems()
    {
        mDragItems.clear();
    }

    bool SortFilterItemModel::filterAccepts(const ItemStack& item)
    {
        MWWorld::Ptr base = item.mBase;

        int category = 0;
        switch (base.getType())
        {
            case ESM::Armor::sRecordId:
            case ESM::Clothing::sRecordId:
                category = Category_Apparel;
                break;
            case ESM::Weapon::sRecordId:
                category = Category_Weapon;
                break;
            case ESM::Ingredient::sRecordId:
            case ESM::Potion::sRecordId:
                category = Category_Magic;
                break;
            case ESM::Miscellaneous::sRecordId:
            case ESM::Repair::sRecordId:
            case ESM::Lockpick::sRecordId:
            case ESM::Light::sRecordId:
            case ESM::Apparatus::sRecordId:
            case ESM::Book::sRecordId:
            case ESM::Probe::sRecordId:
                category = Category_Misc;
                break;
        }

        if (item.mFlags & ItemStack::Flag_Enchanted)
            category |= Category_Magic;

        if (!(category & mCategory))
            return false;

        if (mFilter & Filter_OnlyIngredients)
        {
            if (base.getType() != ESM::Ingredient::sRecordId)
                return false;

            if (!mNameFilter.empty() && !mEffectFilter.empty())
                throw std::logic_error("name and magic effect filter are mutually exclusive");

            if (!mNameFilter.empty())
            {
                const auto itemName = Utf8Stream::lowerCaseUtf8(base.getClass().getName(base));
                return itemName.find(mNameFilter) != std::string::npos;
            }

            if (!mEffectFilter.empty())
            {
                MWWorld::Ptr player = MWBase::Environment::get().getWorld()->getPlayerPtr();
                const auto alchemySkill = player.getClass().getSkill(player, ESM::Skill::Alchemy);

                const auto effects = MWMechanics::Alchemy::effectsDescription(base, alchemySkill);

                for (const auto& effect : effects)
                {
                    const auto ciEffect = Utf8Stream::lowerCaseUtf8(effect);

                    if (ciEffect.find(mEffectFilter) != std::string::npos)
                        return true;
                }
                return false;
            }
            return true;
        }

        if ((mFilter & Filter_OnlyEnchanted) && !(item.mFlags & ItemStack::Flag_Enchanted))
            return false;
        if ((mFilter & Filter_OnlyChargedSoulstones)
            && (base.getType() != ESM::Miscellaneous::sRecordId || base.getCellRef().getSoul().empty()
                || !MWBase::Environment::get().getESMStore()->get<ESM::Creature>().search(base.getCellRef().getSoul())))
            return false;
        if ((mFilter & Filter_OnlyRepairTools) && (base.getType() != ESM::Repair::sRecordId))
            return false;
        if ((mFilter & Filter_OnlyEnchantable)
            && (item.mFlags & ItemStack::Flag_Enchanted
                || (base.getType() != ESM::Armor::sRecordId && base.getType() != ESM::Clothing::sRecordId
                    && base.getType() != ESM::Weapon::sRecordId && base.getType() != ESM::Book::sRecordId)))
            return false;
        if ((mFilter & Filter_OnlyEnchantable) && base.getType() == ESM::Book::sRecordId
            && !base.get<ESM::Book>()->mBase->mData.mIsScroll)
            return false;

        if ((mFilter & Filter_OnlyUsableItems))
        {
            std::unique_ptr<MWWorld::Action> actionOnUse = base.getClass().use(base);
            if (!actionOnUse || actionOnUse->isNullAction())
                return false;
        }

        if ((mFilter & Filter_OnlyRepairable)
            && (!base.getClass().hasItemHealth(base)
                || (base.getClass().getItemHealth(base) >= base.getClass().getItemMaxHealth(base))
                || (base.getType() != ESM::Weapon::sRecordId && base.getType() != ESM::Armor::sRecordId)))
            return false;

        if (mFilter & Filter_OnlyRechargable)
        {
            if (!(item.mFlags & ItemStack::Flag_Enchanted))
                return false;

            const ESM::RefId& enchId = base.getClass().getEnchantment(base);
            const ESM::Enchantment* ench
                = MWBase::Environment::get().getESMStore()->get<ESM::Enchantment>().search(enchId);
            if (!ench)
            {
                Log(Debug::Warning) << "Warning: Can't find enchantment '" << enchId << "' on item "
                                    << base.getCellRef().getRefId();
                return false;
            }

            if (base.getCellRef().getEnchantmentCharge() == -1
                || base.getCellRef().getEnchantmentCharge() >= MWMechanics::getEnchantmentCharge(*ench))
                return false;
        }

        if ((mFilter & Filter_OnlyAlchemyTools))
        {
            if (base.getType() != ESM::Apparatus::sRecordId)
                return false;

            int32_t apparatusType = base.get<ESM::Apparatus>()->mBase->mData.mType;
            if (mApparatusTypeFilter >= 0 && apparatusType != mApparatusTypeFilter)
                return false;
        }

        std::string compare = Utf8Stream::lowerCaseUtf8(item.mBase.getClass().getName(item.mBase));
        if (compare.find(mNameFilter) == std::string::npos)
            return false;

        return true;
    }

    ItemStack SortFilterItemModel::getItem(ModelIndex index)
    {
        if (index < 0)
            throw std::runtime_error("Invalid index supplied");
        if (mItems.size() <= static_cast<size_t>(index))
            throw std::runtime_error("Item index out of range");
        return mItems[index];
    }

    size_t SortFilterItemModel::getItemCount()
    {
        return mItems.size();
    }

    void SortFilterItemModel::setCategory(int category)
    {
        mCategory = category;
    }

    void SortFilterItemModel::setFilter(int filter)
    {
        mFilter = filter;
    }

    void SortFilterItemModel::setNameFilter(const std::string& filter)
    {
        mNameFilter = Utf8Stream::lowerCaseUtf8(filter);
    }

    void SortFilterItemModel::setEffectFilter(const std::string& filter)
    {
        mEffectFilter = Utf8Stream::lowerCaseUtf8(filter);
    }

    void SortFilterItemModel::setApparatusTypeFilter(const int32_t type)
    {
        mApparatusTypeFilter = type;
    }

    void SortFilterItemModel::update()
    {
        mSourceModel->update();

        size_t count = mSourceModel->getItemCount();

        mItems.clear();
        for (size_t i = 0; i < count; ++i)
        {
            ItemStack item = mSourceModel->getItem(static_cast<ModelIndex>(i));

            for (const std::pair<MWWorld::Ptr, size_t>& drag : mDragItems)
            {
                if (item.mBase == drag.first)
                {
                    if (item.mCount < drag.second)
                        throw std::runtime_error("Dragging more than present in the model");
                    item.mCount -= drag.second;
                }
            }

            if (item.mCount > 0 && filterAccepts(item))
                mItems.push_back(item);
        }

        Compare cmp;
        cmp.mSortByType = mSortByType;
        std::sort(mItems.begin(), mItems.end(), cmp);
    }

    void SortFilterItemModel::onClose()
    {
        mSourceModel->onClose();
    }

    bool SortFilterItemModel::onDropItem(const MWWorld::Ptr& item, int count)
    {
        return mSourceModel->onDropItem(item, count);
    }

    bool SortFilterItemModel::onTakeItem(const MWWorld::Ptr& item, int count)
    {
        return mSourceModel->onTakeItem(item, count);
    }
}

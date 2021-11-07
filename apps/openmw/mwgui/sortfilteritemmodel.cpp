#include "sortfilteritemmodel.hpp"

#include <components/misc/stringops.hpp>
#include <components/misc/utf8stream.hpp>
#include <components/debug/debuglog.hpp>
#include <components/esm/loadalch.hpp>
#include <components/esm/loadappa.hpp>
#include <components/esm/loadarmo.hpp>
#include <components/esm/loadbook.hpp>
#include <components/esm/loadclot.hpp>
#include <components/esm/loadingr.hpp>
#include <components/esm/loadlock.hpp>
#include <components/esm/loadligh.hpp>
#include <components/esm/loadmisc.hpp>
#include <components/esm/loadprob.hpp>
#include <components/esm/loadrepa.hpp>
#include <components/esm/loadweap.hpp>
#include <components/esm/loadench.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"

#include "../mwworld/class.hpp"
#include "../mwworld/nullaction.hpp"
#include "../mwworld/esmstore.hpp"

#include "../mwmechanics/alchemy.hpp"

namespace
{
    bool compareType(unsigned int type1, unsigned int type2)
    {
        // this defines the sorting order of types. types that are first in the vector appear before other types.
        std::vector<unsigned int> mapping;
        mapping.emplace_back(ESM::Weapon::sRecordId );
        mapping.emplace_back(ESM::Armor::sRecordId );
        mapping.emplace_back(ESM::Clothing::sRecordId );
        mapping.emplace_back(ESM::Potion::sRecordId );
        mapping.emplace_back(ESM::Ingredient::sRecordId );
        mapping.emplace_back(ESM::Apparatus::sRecordId );
        mapping.emplace_back(ESM::Book::sRecordId );
        mapping.emplace_back(ESM::Light::sRecordId );
        mapping.emplace_back(ESM::Miscellaneous::sRecordId );
        mapping.emplace_back(ESM::Lockpick::sRecordId );
        mapping.emplace_back(ESM::Repair::sRecordId );
        mapping.emplace_back(ESM::Probe::sRecordId );

        assert( std::find(mapping.begin(), mapping.end(), type1) != mapping.end() );
        assert( std::find(mapping.begin(), mapping.end(), type2) != mapping.end() );

        return std::find(mapping.begin(), mapping.end(), type1) < std::find(mapping.begin(), mapping.end(), type2);
    }

    struct Compare
    {
        bool mSortByType;
        Compare() : mSortByType(true) {}
        bool operator() (const MWGui::ItemStack& left, const MWGui::ItemStack& right)
        {
            if (mSortByType && left.mType != right.mType)
                return left.mType < right.mType;

            float result = 0;

            // compare items by type
            auto leftType = left.mBase.getType();
            auto rightType = right.mBase.getType();

            if (leftType != rightType)
                return compareType(leftType, rightType);

            // compare items by name
            std::string leftName = Utf8Stream::lowerCaseUtf8(left.mBase.getClass().getName(left.mBase));
            std::string rightName = Utf8Stream::lowerCaseUtf8(right.mBase.getClass().getName(right.mBase));

            result = leftName.compare(rightName);
            if (result != 0)
                return result < 0;

            // compare items by enchantment:
            // 1. enchanted items showed before non-enchanted
            // 2. item with lesser charge percent comes after items with more charge percent
            // 3. item with constant effect comes before items with non-constant effects
            int leftChargePercent = -1;
            int rightChargePercent = -1;
            leftName = left.mBase.getClass().getEnchantment(left.mBase);
            rightName = right.mBase.getClass().getEnchantment(right.mBase);

            if (!leftName.empty())
            {
                const ESM::Enchantment* ench = MWBase::Environment::get().getWorld()->getStore().get<ESM::Enchantment>().search(leftName);
                if (ench)
                {
                    if (ench->mData.mType == ESM::Enchantment::ConstantEffect)
                        leftChargePercent = 101;
                    else
                        leftChargePercent = static_cast<int>(left.mBase.getCellRef().getNormalizedEnchantmentCharge(ench->mData.mCharge) * 100);
                }
            }

            if (!rightName.empty())
            {
                const ESM::Enchantment* ench = MWBase::Environment::get().getWorld()->getStore().get<ESM::Enchantment>().search(rightName);
                if (ench)
                {
                    if (ench->mData.mType == ESM::Enchantment::ConstantEffect)
                        rightChargePercent = 101;
                    else
                        rightChargePercent = static_cast<int>(right.mBase.getCellRef().getNormalizedEnchantmentCharge(ench->mData.mCharge) * 100);
                }
            }

            result = leftChargePercent - rightChargePercent;
            if (result != 0)
                return result > 0;

            // compare items by condition
            if (left.mBase.getClass().hasItemHealth(left.mBase) && right.mBase.getClass().hasItemHealth(right.mBase))
            {
                result = left.mBase.getClass().getItemHealth(left.mBase) - right.mBase.getClass().getItemHealth(right.mBase);
                if (result != 0)
                    return result > 0;
            }

            // compare items by remaining usage time
            result = left.mBase.getClass().getRemainingUsageTime(left.mBase) - right.mBase.getClass().getRemainingUsageTime(right.mBase);
            if (result != 0)
                return result > 0;

            // compare items by value
            result = left.mBase.getClass().getValue(left.mBase) - right.mBase.getClass().getValue(right.mBase);
            if (result != 0)
                return result > 0;

            // compare items by weight
            result = left.mBase.getClass().getWeight(left.mBase) - right.mBase.getClass().getWeight(right.mBase);
            if (result != 0)
                return result > 0;

            // compare items by Id
            leftName = left.mBase.getCellRef().getRefId();
            rightName = right.mBase.getCellRef().getRefId();

            result = leftName.compare(rightName);
            return result < 0;
        }
    };
}

namespace MWGui
{

    SortFilterItemModel::SortFilterItemModel(ItemModel *sourceModel)
        : mCategory(Category_All)
        , mFilter(0)
        , mSortByType(true)
        , mNameFilter("")
        , mEffectFilter("")
    {
        mSourceModel = sourceModel;
    }

    bool SortFilterItemModel::allowedToUseItems() const
    {
        return mSourceModel->allowedToUseItems();
    }

    void SortFilterItemModel::addDragItem (const MWWorld::Ptr& dragItem, size_t count)
    {
        mDragItems.emplace_back(dragItem, count);
    }

    void SortFilterItemModel::clearDragItems()
    {
        mDragItems.clear();
    }

    bool SortFilterItemModel::filterAccepts (const ItemStack& item)
    {
        MWWorld::Ptr base = item.mBase;

        int category = 0;
        if (base.getType() == ESM::Armor::sRecordId
                || base.getType() == ESM::Clothing::sRecordId)
            category = Category_Apparel;
        else if (base.getType() == ESM::Weapon::sRecordId)
            category = Category_Weapon;
        else if (base.getType() == ESM::Ingredient::sRecordId
                     || base.getType() == ESM::Potion::sRecordId)
            category = Category_Magic;
        else if (base.getType() == ESM::Miscellaneous::sRecordId
                 || base.getType() == ESM::Ingredient::sRecordId
                 || base.getType() == ESM::Repair::sRecordId
                 || base.getType() == ESM::Lockpick::sRecordId
                 || base.getType() == ESM::Light::sRecordId
                 || base.getType() == ESM::Apparatus::sRecordId
                 || base.getType() == ESM::Book::sRecordId
                 || base.getType() == ESM::Probe::sRecordId)
            category = Category_Misc;

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
                MWWorld::Ptr player = MWBase::Environment::get().getWorld ()->getPlayerPtr();
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
        if ((mFilter & Filter_OnlyChargedSoulstones) && (base.getType() != ESM::Miscellaneous::sRecordId
                                                     || base.getCellRef().getSoul() == "" || !MWBase::Environment::get().getWorld()->getStore().get<ESM::Creature>().search(base.getCellRef().getSoul())))
            return false;
        if ((mFilter & Filter_OnlyRepairTools) && (base.getType() != ESM::Repair::sRecordId))
            return false;
        if ((mFilter & Filter_OnlyEnchantable) && (item.mFlags & ItemStack::Flag_Enchanted
                                               || (base.getType() != ESM::Armor::sRecordId
                                                   && base.getType() != ESM::Clothing::sRecordId
                                                   && base.getType() != ESM::Weapon::sRecordId
                                                   && base.getType() != ESM::Book::sRecordId)))
            return false;
        if ((mFilter & Filter_OnlyEnchantable) && base.getType() == ESM::Book::sRecordId
                && !base.get<ESM::Book>()->mBase->mData.mIsScroll)
            return false;

        if ((mFilter & Filter_OnlyUsableItems) && base.getClass().getScript(base).empty())
        {
            std::shared_ptr<MWWorld::Action> actionOnUse = base.getClass().use(base);
            if (!actionOnUse || actionOnUse->isNullAction())
                return false;
        }

        if ((mFilter & Filter_OnlyRepairable) && (
                    !base.getClass().hasItemHealth(base)
                    || (base.getClass().getItemHealth(base) == base.getClass().getItemMaxHealth(base))
                    || (base.getType() != ESM::Weapon::sRecordId
                        && base.getType() != ESM::Armor::sRecordId)))
            return false;

        if (mFilter & Filter_OnlyRechargable)
        {
            if (!(item.mFlags & ItemStack::Flag_Enchanted))
                return false;

            std::string enchId = base.getClass().getEnchantment(base);
            const ESM::Enchantment* ench = MWBase::Environment::get().getWorld()->getStore().get<ESM::Enchantment>().search(enchId);
            if (!ench)
            {
                Log(Debug::Warning) << "Warning: Can't find enchantment '" << enchId << "' on item " << base.getCellRef().getRefId();
                return false;
            }

            if (base.getCellRef().getEnchantmentCharge() >= ench->mData.mCharge
                    || base.getCellRef().getEnchantmentCharge() == -1)
                return false;
        }

        std::string compare = Utf8Stream::lowerCaseUtf8(item.mBase.getClass().getName(item.mBase));
        if(compare.find(mNameFilter) == std::string::npos)
            return false;

        return true;
    }

    ItemStack SortFilterItemModel::getItem (ModelIndex index)
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

    void SortFilterItemModel::setCategory (int category)
    {
        mCategory = category;
    }

    void SortFilterItemModel::setFilter (int filter)
    {
        mFilter = filter;
    }

    void SortFilterItemModel::setNameFilter (const std::string& filter)
    {
        mNameFilter = Utf8Stream::lowerCaseUtf8(filter);
    }

    void SortFilterItemModel::setEffectFilter (const std::string& filter)
    {
        mEffectFilter = Utf8Stream::lowerCaseUtf8(filter);
    }

    void SortFilterItemModel::update()
    {
        mSourceModel->update();

        size_t count = mSourceModel->getItemCount();

        mItems.clear();
        for (size_t i=0; i<count; ++i)
        {
            ItemStack item = mSourceModel->getItem(i);

            for (std::vector<std::pair<MWWorld::Ptr, size_t> >::iterator it = mDragItems.begin(); it != mDragItems.end(); ++it)
            {
                if (item.mBase == it->first)
                {
                    if (item.mCount < it->second)
                        throw std::runtime_error("Dragging more than present in the model");
                    item.mCount -= it->second;
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

    bool SortFilterItemModel::onDropItem(const MWWorld::Ptr &item, int count)
    {
        return mSourceModel->onDropItem(item, count);
    }

    bool SortFilterItemModel::onTakeItem(const MWWorld::Ptr &item, int count)
    {
        return mSourceModel->onTakeItem(item, count);
    }
}

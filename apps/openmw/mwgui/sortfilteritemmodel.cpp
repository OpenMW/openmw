#include "sortfilteritemmodel.hpp"

#include <components/misc/stringops.hpp>

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

#include "../mwworld/class.hpp"
#include "../mwworld/nullaction.hpp"

namespace
{
    bool compareType(const std::string& type1, const std::string& type2)
    {
        // this defines the sorting order of types. types that are first in the vector appear before other types.
        std::vector<std::string> mapping;
        mapping.push_back( typeid(ESM::Weapon).name() );
        mapping.push_back( typeid(ESM::Armor).name() );
        mapping.push_back( typeid(ESM::Clothing).name() );
        mapping.push_back( typeid(ESM::Potion).name() );
        mapping.push_back( typeid(ESM::Ingredient).name() );
        mapping.push_back( typeid(ESM::Apparatus).name() );
        mapping.push_back( typeid(ESM::Book).name() );
        mapping.push_back( typeid(ESM::Light).name() );
        mapping.push_back( typeid(ESM::Miscellaneous).name() );
        mapping.push_back( typeid(ESM::Lockpick).name() );
        mapping.push_back( typeid(ESM::Repair).name() );
        mapping.push_back( typeid(ESM::Probe).name() );

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

            if (left.mBase.getTypeName() == right.mBase.getTypeName())
            {
                std::string leftName = Misc::StringUtils::lowerCase(left.mBase.getClass().getName(left.mBase));
                std::string rightName = Misc::StringUtils::lowerCase(right.mBase.getClass().getName(right.mBase));

                return leftName.compare(rightName) < 0;
            }
            else
                return compareType(left.mBase.getTypeName(), right.mBase.getTypeName());
        }
    };
}

namespace MWGui
{

    SortFilterItemModel::SortFilterItemModel(ItemModel *sourceModel)
        : mCategory(Category_All)
        , mShowEquipped(true)
        , mSortByType(true)
        , mFilter(0)
    {
        mSourceModel = sourceModel;
    }

    void SortFilterItemModel::addDragItem (const MWWorld::Ptr& dragItem, size_t count)
    {
        mDragItems.push_back(std::make_pair(dragItem, count));
    }

    void SortFilterItemModel::clearDragItems()
    {
        mDragItems.clear();
    }

    bool SortFilterItemModel::filterAccepts (const ItemStack& item)
    {
        MWWorld::Ptr base = item.mBase;

        if (item.mType == ItemStack::Type_Equipped && !mShowEquipped)
            return false;

        int category = 0;
        if (base.getTypeName() == typeid(ESM::Armor).name()
                || base.getTypeName() == typeid(ESM::Clothing).name())
            category = Category_Apparel;
        else if (base.getTypeName() == typeid(ESM::Weapon).name())
            category = Category_Weapon;
        else if (base.getTypeName() == typeid(ESM::Ingredient).name()
                     || base.getTypeName() == typeid(ESM::Potion).name())
            category = Category_Magic;
        else if (base.getTypeName() == typeid(ESM::Miscellaneous).name()
                 || base.getTypeName() == typeid(ESM::Ingredient).name()
                 || base.getTypeName() == typeid(ESM::Repair).name()
                 || base.getTypeName() == typeid(ESM::Lockpick).name()
                 || base.getTypeName() == typeid(ESM::Light).name()
                 || base.getTypeName() == typeid(ESM::Apparatus).name()
                 || base.getTypeName() == typeid(ESM::Book).name()
                 || base.getTypeName() == typeid(ESM::Probe).name())
            category = Category_Misc;

        if (item.mFlags & ItemStack::Flag_Enchanted)
            category |= Category_Magic;

        if (!(category & mCategory))
            return false;

        if ((mFilter & Filter_OnlyIngredients) && base.getTypeName() != typeid(ESM::Ingredient).name())
            return false;
        if ((mFilter & Filter_OnlyEnchanted) && !(item.mFlags & ItemStack::Flag_Enchanted))
            return false;
        if ((mFilter & Filter_OnlyChargedSoulstones) && (base.getTypeName() != typeid(ESM::Miscellaneous).name()
                                                     || base.getCellRef().getSoul() == ""))
            return false;
        if ((mFilter & Filter_OnlyEnchantable) && (item.mFlags & ItemStack::Flag_Enchanted
                                               || (base.getTypeName() != typeid(ESM::Armor).name()
                                                   && base.getTypeName() != typeid(ESM::Clothing).name()
                                                   && base.getTypeName() != typeid(ESM::Weapon).name()
                                                   && base.getTypeName() != typeid(ESM::Book).name())))
            return false;
        if ((mFilter & Filter_OnlyEnchantable) && base.getTypeName() == typeid(ESM::Book).name()
                && !base.get<ESM::Book>()->mBase->mData.mIsScroll)
            return false;

        if ((mFilter & Filter_OnlyUsableItems) && typeid(*base.getClass().use(base)) == typeid(MWWorld::NullAction)
                && base.getClass().getScript(base).empty())
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

}

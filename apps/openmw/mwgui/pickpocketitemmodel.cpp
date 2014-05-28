#include "pickpocketitemmodel.hpp"

#include "../mwmechanics/npcstats.hpp"
#include "../mwworld/class.hpp"

namespace MWGui
{

    PickpocketItemModel::PickpocketItemModel(const MWWorld::Ptr& thief, ItemModel *sourceModel)
    {
        mSourceModel = sourceModel;
        int chance = thief.getClass().getSkill(thief, ESM::Skill::Sneak);

        mSourceModel->update();
        for (size_t i = 0; i<mSourceModel->getItemCount(); ++i)
        {
            if (std::rand() / static_cast<float>(RAND_MAX) * 100 > chance)
                mHiddenItems.push_back(mSourceModel->getItem(i));
        }
    }

    ItemStack PickpocketItemModel::getItem (ModelIndex index)
    {
        if (index < 0)
            throw std::runtime_error("Invalid index supplied");
        if (mItems.size() <= static_cast<size_t>(index))
            throw std::runtime_error("Item index out of range");
        return mItems[index];
    }

    size_t PickpocketItemModel::getItemCount()
    {
        return mItems.size();
    }

    void PickpocketItemModel::update()
    {
        mSourceModel->update();
        mItems.clear();
        for (size_t i = 0; i<mSourceModel->getItemCount(); ++i)
        {
            const ItemStack& item = mSourceModel->getItem(i);

            // Bound items may not be stolen
            if (item.mBase.getCellRef().getRefId().size() > 6
                    && item.mBase.getCellRef().getRefId().substr(0,6) == "bound_")
            {
                continue;
            }

            if (std::find(mHiddenItems.begin(), mHiddenItems.end(), item) == mHiddenItems.end()
                    && item.mType != ItemStack::Type_Equipped)
                mItems.push_back(item);
        }
    }

    void PickpocketItemModel::removeItem (const ItemStack &item, size_t count)
    {
        ProxyItemModel::removeItem(item, count);
        /// \todo check if player is detected
    }

}

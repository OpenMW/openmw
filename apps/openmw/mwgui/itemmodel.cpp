#include "itemmodel.hpp"

#include <set>

#include "../mwworld/class.hpp"
#include "../mwworld/containerstore.hpp"
#include "../mwworld/store.hpp"
#include "../mwworld/esmstore.hpp"

#include "../mwbase/world.hpp"
#include "../mwbase/environment.hpp"

namespace MWGui
{

    ItemStack::ItemStack(const MWWorld::Ptr &base, ItemModel *creator, size_t count)
        : mType(Type_Normal)
        , mFlags(0)
        , mCreator(creator)
        , mCount(count)
        , mBase(base)
    {
        if (base.getClass().getEnchantment(base) != "")
            mFlags |= Flag_Enchanted;

        static std::set<std::string> boundItemIDCache;

        // If this is empty then we haven't executed the GMST cache logic yet; or there isn't any sMagicBound* GMST's for some reason
        if (boundItemIDCache.empty())
        {
            // Build a list of known bound item ID's
            const MWWorld::Store<ESM::GameSetting> &gameSettings = MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>();

            for (MWWorld::Store<ESM::GameSetting>::iterator currentIteration = gameSettings.begin(); currentIteration != gameSettings.end(); ++currentIteration)
            {
                const ESM::GameSetting &currentSetting = *currentIteration;
                std::string currentGMSTID = currentSetting.mId;
                Misc::StringUtils::lowerCaseInPlace(currentGMSTID);

                // Don't bother checking this GMST if it's not a sMagicBound* one.
                const std::string& toFind = "smagicbound";
                if (currentGMSTID.compare(0, toFind.length(), toFind) != 0)
                    continue;

                // All sMagicBound* GMST's should be of type string
                std::string currentGMSTValue = currentSetting.getString();
                Misc::StringUtils::lowerCaseInPlace(currentGMSTValue);

                boundItemIDCache.insert(currentGMSTValue);
            }
        }

        // Perform bound item check and assign the Flag_Bound bit if it passes
        std::string tempItemID = base.getCellRef().getRefId();
        Misc::StringUtils::lowerCaseInPlace(tempItemID);

        if (boundItemIDCache.count(tempItemID) != 0)
            mFlags |= Flag_Bound;
    }

    ItemStack::ItemStack()
        : mType(Type_Normal)
        , mFlags(0)
        , mCreator(NULL)
        , mCount(0)
    {
    }

    bool ItemStack::stacks(const ItemStack &other)
    {
        if(mBase == other.mBase)
            return true;

        // If one of the items is in an inventory and currently equipped, we need to check stacking both ways to be sure
        if (mBase.getContainerStore() && other.mBase.getContainerStore())
            return mBase.getContainerStore()->stacks(mBase, other.mBase)
                    && other.mBase.getContainerStore()->stacks(mBase, other.mBase);

        if (mBase.getContainerStore())
            return mBase.getContainerStore()->stacks(mBase, other.mBase);
        if (other.mBase.getContainerStore())
            return other.mBase.getContainerStore()->stacks(mBase, other.mBase);

        MWWorld::ContainerStore store;
        return store.stacks(mBase, other.mBase);

    }

    bool operator == (const ItemStack& left, const ItemStack& right)
    {
        if (left.mType != right.mType)
            return false;

        if(left.mBase == right.mBase)
            return true;

        // If one of the items is in an inventory and currently equipped, we need to check stacking both ways to be sure
        if (left.mBase.getContainerStore() && right.mBase.getContainerStore())
            return left.mBase.getContainerStore()->stacks(left.mBase, right.mBase)
                    && right.mBase.getContainerStore()->stacks(left.mBase, right.mBase);

        if (left.mBase.getContainerStore())
            return left.mBase.getContainerStore()->stacks(left.mBase, right.mBase);
        if (right.mBase.getContainerStore())
            return right.mBase.getContainerStore()->stacks(left.mBase, right.mBase);

        MWWorld::ContainerStore store;
        return store.stacks(left.mBase, right.mBase);
    }

    ItemModel::ItemModel()
    {
    }

    MWWorld::Ptr ItemModel::moveItem(const ItemStack &item, size_t count, ItemModel *otherModel)
    {
        MWWorld::Ptr ret = otherModel->copyItem(item, count);
        removeItem(item, count);
        return ret;
    }

    bool ItemModel::allowedToInsertItems() const
    {
        return true;
    }


    ProxyItemModel::ProxyItemModel()
        : mSourceModel(NULL)
    {
    }

    ProxyItemModel::~ProxyItemModel()
    {
        delete mSourceModel;
    }

    MWWorld::Ptr ProxyItemModel::copyItem (const ItemStack& item, size_t count, bool setNewOwner)
    {
        return mSourceModel->copyItem (item, count, setNewOwner);
    }

    void ProxyItemModel::removeItem (const ItemStack& item, size_t count)
    {
        mSourceModel->removeItem (item, count);
    }

    ItemModel::ModelIndex ProxyItemModel::mapToSource (ModelIndex index)
    {
        const ItemStack& itemToSearch = getItem(index);
        for (size_t i=0; i<mSourceModel->getItemCount(); ++i)
        {
            const ItemStack& item = mSourceModel->getItem(i);
            if (item.mBase == itemToSearch.mBase)
                return i;
        }
        return -1;
    }

    ItemModel::ModelIndex ProxyItemModel::mapFromSource (ModelIndex index)
    {
        const ItemStack& itemToSearch = mSourceModel->getItem(index);
        for (size_t i=0; i<getItemCount(); ++i)
        {
            const ItemStack& item = getItem(i);
            if (item.mBase == itemToSearch.mBase)
                return i;
        }
        return -1;
    }

    ItemModel::ModelIndex ProxyItemModel::getIndex (ItemStack item)
    {
        return mSourceModel->getIndex(item);
    }

    void ProxyItemModel::setSourceModel(ItemModel *sourceModel)
    {
        if (mSourceModel == sourceModel)
            return;

        if (mSourceModel)
        {
            delete mSourceModel;
            mSourceModel = NULL;
        }

        mSourceModel = sourceModel;
    }

}

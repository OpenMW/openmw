#include "itemmodel.hpp"

#include "../mwworld/class.hpp"
#include "../mwworld/containerstore.hpp"

namespace MWGui
{

    ItemStack::ItemStack(const MWWorld::Ptr &base, ItemModel *creator, size_t count)
        : mCreator(creator)
        , mCount(count)
        , mFlags(0)
        , mType(Type_Normal)
        , mBase(base)
    {
        assert(base.getContainerStore());

        if (MWWorld::Class::get(base).getEnchantment(base) != "")
            mFlags |= Flag_Enchanted;
    }

    ItemStack::ItemStack()
        : mCreator(NULL)
        , mCount(0)
        , mFlags(0)
        , mType(Type_Normal)
    {
    }

    bool ItemStack::stacks(const ItemStack &other)
    {
        if(mBase == other.mBase)
            return true;
        return mBase.getContainerStore()->stacks(mBase, other.mBase)
                && other.mBase.getContainerStore()->stacks(mBase, other.mBase);
    }

    bool operator == (const ItemStack& left, const ItemStack& right)
    {
        if (left.mType != right.mType)
            return false;
        if(left.mBase == right.mBase)
            return true;
        return left.mBase.getContainerStore()->stacks(left.mBase, right.mBase)
                && right.mBase.getContainerStore()->stacks(left.mBase, right.mBase);
    }

    ItemModel::ItemModel()
    {
    }


    ProxyItemModel::~ProxyItemModel()
    {
        delete mSourceModel;
    }

    void ProxyItemModel::copyItem (const ItemStack& item, size_t count)
    {
        // no need to use mapToSource since itemIndex refers to an index in the sourceModel
        mSourceModel->copyItem (item, count);
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
            if (item == itemToSearch)
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
            if (item == itemToSearch)
                return i;
        }
        return -1;
    }

    ItemModel::ModelIndex ProxyItemModel::getIndex (ItemStack item)
    {
        return mSourceModel->getIndex(item);
    }

}

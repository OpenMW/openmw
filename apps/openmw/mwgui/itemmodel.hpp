#ifndef MWGUI_ITEM_MODEL_H
#define MWGUI_ITEM_MODEL_H

#include "../mwworld/ptr.hpp"

namespace MWGui
{

    class ItemModel;

    /// @brief A single item stack managed by an item model
    struct ItemStack
    {
        ItemStack (const MWWorld::Ptr& base, ItemModel* creator, size_t count);
        ItemStack();
        bool stacks (const ItemStack& other);
        ///< like operator==, only without checking mType

        enum Type
        {
            Type_Barter,
            Type_Equipped,
            Type_Normal
        };
        Type mType;

        enum Flags
        {
            Flag_Enchanted = (1<<0)
        };
        int mFlags;

        ItemModel* mCreator;
        size_t mCount;
        MWWorld::Ptr mBase;
    };

    bool operator == (const ItemStack& left, const ItemStack& right);


    /// @brief The base class that all item models should derive from.
    class ItemModel
    {
    public:
        ItemModel();
        virtual ~ItemModel() {}

        typedef int ModelIndex;

        virtual ItemStack getItem (ModelIndex index) = 0;
        ///< throws for invalid index
        virtual size_t getItemCount() = 0;

        virtual ModelIndex getIndex (ItemStack item) = 0;

        virtual void update() = 0;

        /// Move items from this model to \a otherModel.
        virtual MWWorld::Ptr moveItem (const ItemStack& item, size_t count, ItemModel* otherModel);

        /// @param setNewOwner Set the copied item's owner to the actor we are copying to, or keep the original owner?
        virtual MWWorld::Ptr copyItem (const ItemStack& item, size_t count, bool setNewOwner=false) = 0;
        virtual void removeItem (const ItemStack& item, size_t count) = 0;

    private:
        ItemModel(const ItemModel&);
        ItemModel& operator=(const ItemModel&);
    };

    /// @brief A proxy item model can be used to filter or rearrange items from a source model (or even add new items to it).
    /// The neat thing is that this does not actually alter the source model.
    class ProxyItemModel : public ItemModel
    {
    public:
        virtual ~ProxyItemModel();
        virtual MWWorld::Ptr copyItem (const ItemStack& item, size_t count, bool setNewOwner=false);
        virtual void removeItem (const ItemStack& item, size_t count);
        virtual ModelIndex getIndex (ItemStack item);

        ModelIndex mapToSource (ModelIndex index);
        ModelIndex mapFromSource (ModelIndex index);
    protected:
        ItemModel* mSourceModel;
    };

}

#endif

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
            Flag_Enchanted = (1<<0),
            Flag_Bound = (1<<1)
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

        typedef int ModelIndex; // -1 means invalid index

        /// Throws for invalid index or out of range index
        virtual ItemStack getItem (ModelIndex index) = 0;

        /// The number of items in the model, this specifies the range of indices you can pass to
        /// the getItem function (but this range is only valid until the next call to update())
        virtual size_t getItemCount() = 0;

        /// Returns an invalid index if the item was not found
        virtual ModelIndex getIndex (ItemStack item) = 0;

        /// Rebuild the item model, this will invalidate existing model indices
        virtual void update() = 0;

        /// Move items from this model to \a otherModel.
        /// @note Derived implementations may return an empty Ptr if the move was unsuccessful.
        virtual MWWorld::Ptr moveItem (const ItemStack& item, size_t count, ItemModel* otherModel);

        virtual MWWorld::Ptr copyItem (const ItemStack& item, size_t count, bool allowAutoEquip = true) = 0;
        virtual void removeItem (const ItemStack& item, size_t count) = 0;

        /// Is the player allowed to use items from this item model? (default true)
        virtual bool allowedToUseItems() const;
        virtual void onClose()
        {
        }
        virtual bool onDropItem(const MWWorld::Ptr &item, int count);
        virtual bool onTakeItem(const MWWorld::Ptr &item, int count);

    private:
        ItemModel(const ItemModel&);
        ItemModel& operator=(const ItemModel&);
    };

    /// @brief A proxy item model can be used to filter or rearrange items from a source model (or even add new items to it).
    /// The neat thing is that this does not actually alter the source model.
    class ProxyItemModel : public ItemModel
    {
    public:
        ProxyItemModel();
        virtual ~ProxyItemModel();

        bool allowedToUseItems() const;

        virtual void onClose();
        virtual bool onDropItem(const MWWorld::Ptr &item, int count);
        virtual bool onTakeItem(const MWWorld::Ptr &item, int count);

        virtual MWWorld::Ptr copyItem (const ItemStack& item, size_t count, bool allowAutoEquip = true);
        virtual void removeItem (const ItemStack& item, size_t count);
        virtual ModelIndex getIndex (ItemStack item);

        /// @note Takes ownership of the passed pointer.
        void setSourceModel(ItemModel* sourceModel);

        ModelIndex mapToSource (ModelIndex index);
        ModelIndex mapFromSource (ModelIndex index);
    protected:
        ItemModel* mSourceModel;
    };

}

#endif

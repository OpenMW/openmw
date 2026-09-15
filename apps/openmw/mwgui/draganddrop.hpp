#ifndef OPENMW_MWGUI_DRAGANDDROP_H
#define OPENMW_MWGUI_DRAGANDDROP_H

#include "itemmodel.hpp"
#include "itemwidget.hpp"

#include <cstddef>

namespace MyGUI
{
    class Widget;
}

namespace MWGui
{

    class ItemView;
    class SortFilterItemModel;

    class DragAndDrop
    {
    public:
        bool mIsOnDragAndDrop;
        ItemWidget* mDraggedWidget;
        ItemModel* mSourceModel;
        ItemView* mSourceView;
        SortFilterItemModel* mSourceSortModel;
        ItemStack mItem;
        std::size_t mDraggedCount;

        DragAndDrop();

        void startDrag(int index, SortFilterItemModel* sortModel, ItemModel* sourceModel, ItemView* sourceView,
            std::size_t count, bool playSound = true);
        void drop(ItemModel* targetModel, ItemView* targetView, bool playSound = true);
        /// Synchronize the dragged count, subtracting consumedCount items used from the cursor.
        void update(std::size_t consumedCount = 0);
        void onFrame();

        void finish();
    };

}

#endif

#ifndef OPENMW_MWGUI_DRAGANDDROP_H
#define OPENMW_MWGUI_DRAGANDDROP_H

#include "itemmodel.hpp"

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
        MyGUI::Widget* mDraggedWidget;
        ItemModel* mSourceModel;
        ItemView* mSourceView;
        SortFilterItemModel* mSourceSortModel;
        ItemStack mItem;
        int mDraggedCount;

        DragAndDrop();

        void startDrag (int index, SortFilterItemModel* sortModel, ItemModel* sourceModel, ItemView* sourceView, int count);
        void drop (ItemModel* targetModel, ItemView* targetView);
        void onFrame();

        void finish();
    };

}

#endif

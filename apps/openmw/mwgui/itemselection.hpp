#include "container.hpp"

namespace MWGui
{
    class ItemView;
    class SortFilterItemModel;
    class InventoryItemModel;

    class ItemSelectionDialog : public WindowModal
    {
    public:
        ItemSelectionDialog(const std::string& label);

        typedef MyGUI::delegates::CMultiDelegate0 EventHandle_Void;
        typedef MyGUI::delegates::CMultiDelegate1<MWWorld::Ptr> EventHandle_Item;

        EventHandle_Item eventItemSelected;
        EventHandle_Void eventDialogCanceled;

        void openContainer (const MWWorld::Ptr& container);
        void setCategory(int category);
        void setFilter(int filter);

    private:
        ItemView* mItemView;
        SortFilterItemModel* mSortModel;
        InventoryItemModel* mModel;

        void onSelectedItem(int index);

        void onCancelButtonClicked(MyGUI::Widget* sender);
    };

}

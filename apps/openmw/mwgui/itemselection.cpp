#include "itemselection.hpp"

#include <MyGUI_TextBox.h>
#include <MyGUI_Button.h>

#include "itemview.hpp"
#include "inventoryitemmodel.hpp"
#include "sortfilteritemmodel.hpp"

namespace MWGui
{

    ItemSelectionDialog::ItemSelectionDialog(const std::string &label)
        : WindowModal("openmw_itemselection_dialog.layout")
        , mSortModel(nullptr)
        , mModel(nullptr)
    {
        getWidget(mItemView, "ItemView");
        mItemView->eventItemClicked += MyGUI::newDelegate(this, &ItemSelectionDialog::onSelectedItem);

        MyGUI::TextBox* l;
        getWidget(l, "Label");
        l->setCaptionWithReplacing (label);

        MyGUI::Button* cancelButton;
        getWidget(cancelButton, "CancelButton");
        cancelButton->eventMouseButtonClick += MyGUI::newDelegate(this, &ItemSelectionDialog::onCancelButtonClicked);

        center();
    }

    bool ItemSelectionDialog::exit()
    {
        eventDialogCanceled();
        return true;
    }

    void ItemSelectionDialog::openContainer(const MWWorld::Ptr& container)
    {
        mModel = new InventoryItemModel(container);
        mSortModel = new SortFilterItemModel(mModel);
        mItemView->setModel(mSortModel);
        mItemView->resetScrollBars();
    }

    void ItemSelectionDialog::setCategory(int category)
    {
        mSortModel->setCategory(category);
        mItemView->update();
    }

    void ItemSelectionDialog::setFilter(int filter)
    {
        mSortModel->setFilter(filter);
        mItemView->update();
    }

    void ItemSelectionDialog::onSelectedItem(int index)
    {
        ItemStack item = mSortModel->getItem(index);
        eventItemSelected(item.mBase);
    }

    void ItemSelectionDialog::onCancelButtonClicked(MyGUI::Widget* sender)
    {
        exit();
    }

}

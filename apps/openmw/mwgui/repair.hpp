#ifndef OPENMW_MWGUI_REPAIR_H
#define OPENMW_MWGUI_REPAIR_H

#include <memory>

#include <apps/openmw/mwgui/itemselection.hpp>

#include "windowbase.hpp"

#include "../mwmechanics/repair.hpp"

namespace MWWorld
{
    class Ptr;
}

namespace MyGUI
{
    class Button;
    class TextBox;
    class Widget;
}

namespace MWGui
{

    class ItemWidget;
    class ItemChargeView;

    class Repair : public WindowBase
    {
    public:
        Repair();

        void onOpen() override;

        void setPtr(const MWWorld::Ptr& item) override;

    protected:
        ItemChargeView* mRepairBox;

        MyGUI::Widget* mToolBox;

        ItemWidget* mToolIcon;

        std::unique_ptr<ItemSelectionDialog> mItemSelectionDialog;

        MyGUI::TextBox* mUsesLabel;
        MyGUI::TextBox* mQualityLabel;

        MyGUI::Button* mCancelButton;

        MWMechanics::Repair mRepair;

        void updateRepairView();

        void onSelectItem(MyGUI::Widget* sender);

        void onItemSelected(MWWorld::Ptr item);
        void onItemCancel();

        void onRepairItem(MyGUI::Widget* sender, const MWWorld::Ptr& ptr);
        void onCancel(MyGUI::Widget* sender);
    };

}

#endif

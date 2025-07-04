#ifndef OPENMW_MWGUI_REPAIR_H
#define OPENMW_MWGUI_REPAIR_H

#include <memory>

#include "windowbase.hpp"

#include "../mwmechanics/repair.hpp"

namespace MWGui
{

    class ItemSelectionDialog;
    class ItemWidget;
    class ItemChargeView;

    class Repair : public WindowBase
    {
    public:
        Repair();

        void onOpen() override;

        void setPtr(const MWWorld::Ptr& item) override;

        std::string_view getWindowIdForLua() const override { return "Repair"; }

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

        bool onControllerButtonEvent(const SDL_ControllerButtonEvent& arg) override;
    };

}

#endif

#ifndef OPENMW_MWGUI_REPAIR_H
#define OPENMW_MWGUI_REPAIR_H

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

    void setPtr (const MWWorld::Ptr& item) override;

protected:
    ItemChargeView* mRepairBox;

    MyGUI::Widget* mToolBox;

    ItemWidget* mToolIcon;

    ItemSelectionDialog* mItemSelectionDialog;

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

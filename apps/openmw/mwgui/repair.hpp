#ifndef OPENMW_MWGUI_REPAIR_H
#define OPENMW_MWGUI_REPAIR_H

#include "windowbase.hpp"

#include "itemselection.hpp"

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

    virtual void onOpen();

    void setPtr (const MWWorld::Ptr& item);

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

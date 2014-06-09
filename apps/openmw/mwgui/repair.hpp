#ifndef OPENMW_MWGUI_REPAIR_H
#define OPENMW_MWGUI_REPAIR_H

#include "windowbase.hpp"

#include "../mwmechanics/repair.hpp"

namespace MWGui
{

class ItemWidget;

class Repair : public WindowBase
{
public:
    Repair();

    virtual void open();

    virtual void exit();

    void startRepairItem (const MWWorld::Ptr& item);

protected:
    MyGUI::Widget* mRepairBox;
    MyGUI::ScrollView* mRepairView;

    MyGUI::Widget* mToolBox;

    ItemWidget* mToolIcon;

    MyGUI::TextBox* mUsesLabel;
    MyGUI::TextBox* mQualityLabel;

    MyGUI::Button* mCancelButton;

    MWMechanics::Repair mRepair;

    void updateRepairView();

    void onRepairItem (MyGUI::Widget* sender);
    void onCancel (MyGUI::Widget* sender);
    void onMouseWheel(MyGUI::Widget* _sender, int _rel);

};

}

#endif

#ifndef OPENMW_MWGUI_REPAIR_H
#define OPENMW_MWGUI_REPAIR_H

#include "window_base.hpp"

#include "../mwworld/ptr.hpp"
#include "../mwmechanics/repair.hpp"

namespace MWGui
{

class Repair : public WindowBase
{
public:
    Repair(MWBase::WindowManager &parWindowManager);

    virtual void open();

    void startRepairItem (const MWWorld::Ptr& item);

protected:
    MyGUI::Widget* mRepairBox;
    MyGUI::ScrollView* mRepairView;

    MyGUI::Widget* mToolBox;

    MyGUI::ImageBox* mToolIcon;

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

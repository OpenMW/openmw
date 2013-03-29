#ifndef OPENMW_MWGUI_MERCHANTREPAIR_H
#define OPENMW_MWGUI_MERCHANTREPAIR_H

#include "window_base.hpp"
#include "../mwworld/ptr.hpp"



namespace MWGui
{

class MerchantRepair : public WindowBase
{
public:
    MerchantRepair(MWBase::WindowManager &parWindowManager);

    virtual void open();

    void startRepair(const MWWorld::Ptr& actor);

private:
    MyGUI::ScrollView* mList;
    MyGUI::Button* mOkButton;
    MyGUI::TextBox* mGoldLabel;

    MWWorld::Ptr mActor;

protected:
    void onMouseWheel(MyGUI::Widget* _sender, int _rel);
    void onRepairButtonClick(MyGUI::Widget* sender);
    void onOkButtonClick(MyGUI::Widget* sender);

};

}

#endif

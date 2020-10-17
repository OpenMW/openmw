#ifndef OPENMW_MWGUI_MERCHANTREPAIR_H
#define OPENMW_MWGUI_MERCHANTREPAIR_H

#include "windowbase.hpp"
#include "../mwworld/ptr.hpp"

namespace MWGui
{

class MerchantRepair : public WindowBase
{
public:
    MerchantRepair();

    void onOpen() override;

    void setPtr(const MWWorld::Ptr& actor) override;

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

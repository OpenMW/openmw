#ifndef OPENMW_MWGUI_RECHARGE_H
#define OPENMW_MWGUI_RECHARGE_H

#include "windowbase.hpp"

namespace MWWorld
{
    class Ptr;
}

namespace MWGui
{

class ItemWidget;
class ItemChargeView;

class Recharge : public WindowBase
{
public:
    Recharge();

    virtual void open();

    virtual void exit();

    void start (const MWWorld::Ptr& gem);

protected:
    ItemChargeView* mBox;

    MyGUI::Widget* mGemBox;

    ItemWidget* mGemIcon;

    MyGUI::TextBox* mChargeLabel;

    MyGUI::Button* mCancelButton;

    void updateView();

    void onItemClicked (MyGUI::Widget* sender, const MWWorld::Ptr& item);
    void onCancel (MyGUI::Widget* sender);
    void onMouseWheel(MyGUI::Widget* _sender, int _rel);

};

}

#endif

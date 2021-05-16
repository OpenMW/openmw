#ifndef OPENMW_MWGUI_RECHARGE_H
#define OPENMW_MWGUI_RECHARGE_H

#include "windowbase.hpp"

namespace MWWorld
{
    class Ptr;
}

namespace MWGui
{

class ItemSelectionDialog;
class ItemWidget;
class ItemChargeView;

class Recharge : public WindowBase
{
public:
    Recharge();

    void onOpen() override;

    void setPtr (const MWWorld::Ptr& gem) override;

protected:
    ItemChargeView* mBox;

    MyGUI::Widget* mGemBox;

    ItemWidget* mGemIcon;

    ItemSelectionDialog* mItemSelectionDialog;

    MyGUI::TextBox* mChargeLabel;

    MyGUI::Button* mCancelButton;

    void updateView();

    void onSelectItem(MyGUI::Widget* sender);

    void onItemSelected(MWWorld::Ptr item);
    void onItemCancel();

    void onItemClicked (MyGUI::Widget* sender, const MWWorld::Ptr& item);
    void onCancel (MyGUI::Widget* sender);
    void onMouseWheel(MyGUI::Widget* _sender, int _rel);

};

}

#endif

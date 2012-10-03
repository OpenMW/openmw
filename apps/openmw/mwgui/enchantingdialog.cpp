#include "enchantingdialog.hpp"


namespace MWGui
{


    EnchantingDialog::EnchantingDialog(MWBase::WindowManager &parWindowManager)
        : WindowBase("openmw_enchanting_dialog.layout", parWindowManager)
    {
        getWidget(mCancelButton, "CancelButton");

        mCancelButton->eventMouseButtonClick += MyGUI::newDelegate(this, &EnchantingDialog::onCancelButtonClicked);
    }

    void EnchantingDialog::open()
    {
        center();
    }

    void EnchantingDialog::startEnchanting (MWWorld::Ptr actor)
    {
        mPtr = actor;
    }

    void EnchantingDialog::onReferenceUnavailable ()
    {
        mWindowManager.removeGuiMode (GM_Dialogue);
        mWindowManager.removeGuiMode (GM_Enchanting);
    }

    void EnchantingDialog::onCancelButtonClicked(MyGUI::Widget* sender)
    {
        mWindowManager.removeGuiMode (GM_Enchanting);
    }
}

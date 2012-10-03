#include "enchantingdialog.hpp"


namespace MWGui
{


    EnchantingDialog::EnchantingDialog(MWBase::WindowManager &parWindowManager)
        : WindowBase("openmw_enchanting_dialog.layout", parWindowManager)
        , EffectEditorBase(parWindowManager)
    {
        getWidget(mCancelButton, "CancelButton");
        getWidget(mAvailableEffectsList, "AvailableEffects");
        getWidget(mUsedEffectsView, "UsedEffects");

        setWidgets(mAvailableEffectsList, mUsedEffectsView);

        mCancelButton->eventMouseButtonClick += MyGUI::newDelegate(this, &EnchantingDialog::onCancelButtonClicked);
    }

    void EnchantingDialog::open()
    {
        center();
    }

    void EnchantingDialog::startEnchanting (MWWorld::Ptr actor)
    {
        mPtr = actor;

        startEditing ();
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

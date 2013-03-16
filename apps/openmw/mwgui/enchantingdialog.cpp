#include "enchantingdialog.hpp"

#include <boost/lexical_cast.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwworld/player.hpp"

#include "itemselection.hpp"
#include "container.hpp"

namespace MWGui
{


    EnchantingDialog::EnchantingDialog(MWBase::WindowManager &parWindowManager)
        : WindowBase("openmw_enchanting_dialog.layout", parWindowManager)
        , EffectEditorBase(parWindowManager)
        , mItemSelectionDialog(NULL)
        , mCurrentEnchantmentPoints(0)
    {
        getWidget(mCancelButton, "CancelButton");
        getWidget(mAvailableEffectsList, "AvailableEffects");
        getWidget(mUsedEffectsView, "UsedEffects");
        getWidget(mItemBox, "ItemBox");
        getWidget(mSoulBox, "SoulBox");
        getWidget(mEnchantmentPoints, "Enchantment");
        getWidget(mCastCost, "CastCost");
        getWidget(mCharge, "Charge");
        getWidget(mTypeButton, "TypeButton");
        getWidget(mBuyButton, "BuyButton");
        getWidget(mPrice, "PriceLabel");

        setWidgets(mAvailableEffectsList, mUsedEffectsView);

        mCancelButton->eventMouseButtonClick += MyGUI::newDelegate(this, &EnchantingDialog::onCancelButtonClicked);
        mItemBox->eventMouseButtonClick += MyGUI::newDelegate(this, &EnchantingDialog::onSelectItem);
        mSoulBox->eventMouseButtonClick += MyGUI::newDelegate(this, &EnchantingDialog::onSelectSoul);
    }

    EnchantingDialog::~EnchantingDialog()
    {
        delete mItemSelectionDialog;
    }

    void EnchantingDialog::open()
    {
        center();
        onRemoveItem(NULL);
        onRemoveSoul(NULL);
    }

    void EnchantingDialog::updateLabels()
    {
        mEnchantmentPoints->setCaption(boost::lexical_cast<std::string>(mCurrentEnchantmentPoints)
                                       + " / " + (mItem.isEmpty() ? "0" : boost::lexical_cast<std::string>(
            MWWorld::Class::get(mItem).getEnchantmentPoints(mItem))));
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

    void EnchantingDialog::onSelectItem(MyGUI::Widget *sender)
    {
        delete mItemSelectionDialog;
        mItemSelectionDialog = new ItemSelectionDialog("#{sEnchantItems}",
            ContainerBase::Filter_Apparel|ContainerBase::Filter_Weapon|ContainerBase::Filter_NoMagic, mWindowManager);
        mItemSelectionDialog->eventItemSelected += MyGUI::newDelegate(this, &EnchantingDialog::onItemSelected);
        mItemSelectionDialog->eventDialogCanceled += MyGUI::newDelegate(this, &EnchantingDialog::onItemCancel);
        mItemSelectionDialog->setVisible(true);
        mItemSelectionDialog->openContainer(MWBase::Environment::get().getWorld()->getPlayer().getPlayer());
        mItemSelectionDialog->drawItems ();
    }

    void EnchantingDialog::onItemSelected(MWWorld::Ptr item)
    {
        mItemSelectionDialog->setVisible(false);

        while (mItemBox->getChildCount ())
            MyGUI::Gui::getInstance ().destroyWidget (mItemBox->getChildAt(0));

        MyGUI::ImageBox* image = mItemBox->createWidget<MyGUI::ImageBox>("ImageBox", MyGUI::IntCoord(0, 0, 32, 32), MyGUI::Align::Default);
        std::string path = std::string("icons\\");
        path += MWWorld::Class::get(item).getInventoryIcon(item);
        int pos = path.rfind(".");
        path.erase(pos);
        path.append(".dds");
        image->setImageTexture (path);
        image->setUserString ("ToolTipType", "ItemPtr");
        image->setUserData(item);
        image->eventMouseButtonClick += MyGUI::newDelegate(this, &EnchantingDialog::onRemoveItem);

        mItem = item;
        updateLabels();
    }

    void EnchantingDialog::onRemoveItem(MyGUI::Widget *sender)
    {
        while (mItemBox->getChildCount ())
            MyGUI::Gui::getInstance ().destroyWidget (mItemBox->getChildAt(0));
        mItem = MWWorld::Ptr();
        updateLabels();
    }

    void EnchantingDialog::onItemCancel()
    {
        mItemSelectionDialog->setVisible(false);
    }

    void EnchantingDialog::onSoulSelected(MWWorld::Ptr item)
    {
        mItemSelectionDialog->setVisible(false);

        while (mSoulBox->getChildCount ())
            MyGUI::Gui::getInstance ().destroyWidget (mSoulBox->getChildAt(0));

        MyGUI::ImageBox* image = mSoulBox->createWidget<MyGUI::ImageBox>("ImageBox", MyGUI::IntCoord(0, 0, 32, 32), MyGUI::Align::Default);
        std::string path = std::string("icons\\");
        path += MWWorld::Class::get(item).getInventoryIcon(item);
        int pos = path.rfind(".");
        path.erase(pos);
        path.append(".dds");
        image->setImageTexture (path);
        image->setUserString ("ToolTipType", "ItemPtr");
        image->setUserData(item);
        image->eventMouseButtonClick += MyGUI::newDelegate(this, &EnchantingDialog::onRemoveSoul);

        mSoul = item;
        updateLabels();
    }

    void EnchantingDialog::onRemoveSoul(MyGUI::Widget *sender)
    {
        while (mSoulBox->getChildCount ())
            MyGUI::Gui::getInstance ().destroyWidget (mSoulBox->getChildAt(0));
        mSoul = MWWorld::Ptr();
        updateLabels();
    }

    void EnchantingDialog::onSoulCancel()
    {
        mItemSelectionDialog->setVisible(false);
    }

    void EnchantingDialog::onSelectSoul(MyGUI::Widget *sender)
    {
        delete mItemSelectionDialog;
        mItemSelectionDialog = new ItemSelectionDialog("#{sSoulGemsWithSouls}",
            ContainerBase::Filter_Misc|ContainerBase::Filter_ChargedSoulstones, mWindowManager);
        mItemSelectionDialog->eventItemSelected += MyGUI::newDelegate(this, &EnchantingDialog::onSoulSelected);
        mItemSelectionDialog->eventDialogCanceled += MyGUI::newDelegate(this, &EnchantingDialog::onSoulCancel);
        mItemSelectionDialog->setVisible(true);
        mItemSelectionDialog->openContainer(MWBase::Environment::get().getWorld()->getPlayer().getPlayer());
        mItemSelectionDialog->drawItems ();

        //mWindowManager.messageBox("#{sInventorySelectNoSoul}");
    }
}

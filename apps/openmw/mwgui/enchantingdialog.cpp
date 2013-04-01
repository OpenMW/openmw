#include "enchantingdialog.hpp"

#include <boost/lexical_cast.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwworld/player.hpp"
#include "../mwworld/manualref.hpp"

#include "itemselection.hpp"
#include "container.hpp"
#include "inventorywindow.hpp"

namespace MWGui
{


    EnchantingDialog::EnchantingDialog(MWBase::WindowManager &parWindowManager)
        : WindowBase("openmw_enchanting_dialog.layout", parWindowManager)
        , EffectEditorBase(parWindowManager)
        , mItemSelectionDialog(NULL)
    {
        getWidget(mName, "NameEdit");
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
        mBuyButton->eventMouseButtonClick += MyGUI::newDelegate(this, &EnchantingDialog::onBuyButtonClicked);
        mTypeButton->eventMouseButtonClick += MyGUI::newDelegate(this, &EnchantingDialog::onTypeButtonClicked);
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
        mEnchantmentPoints->setCaption(boost::lexical_cast<std::string>(mEnchanting.getEnchantCost())
                                       + " / " + boost::lexical_cast<std::string>(mEnchanting.getMaxEnchantValue()));

        mCharge->setCaption(boost::lexical_cast<std::string>(mEnchanting.getGemCharge()));

        mCastCost->setCaption(boost::lexical_cast<std::string>(mEnchanting.getEnchantCost()));

        switch(mEnchanting.getEnchantType())
        {
            case 0:
                mTypeButton->setCaption(mWindowManager.getGameSettingString("sItemCastOnce","Cast Once"));
                mAddEffectDialog.constantEffect=false;
                break;
            case 1:
                mTypeButton->setCaption(mWindowManager.getGameSettingString("sItemCastWhenStrikes", "When Strikes"));
                mAddEffectDialog.constantEffect=false;
                break;
            case 2:
                mTypeButton->setCaption(mWindowManager.getGameSettingString("sItemCastWhenUsed", "When Used"));
                mAddEffectDialog.constantEffect=false;
                break;
            case 3:
                mTypeButton->setCaption(mWindowManager.getGameSettingString("sItemCastConstant", "Cast Constant"));
                mAddEffectDialog.constantEffect=true;
                break;
        }
    }

    void EnchantingDialog::startEnchanting (MWWorld::Ptr actor)
    {

        /*Now there's no need to use other enchanters, player is the enchanter here,
          even if the enchanted object is created by NPC. Could be changed later, probably
          with some price formulas                                                       */
        MWWorld::Ptr player = MWBase::Environment::get().getWorld()->getPlayer().getPlayer();

        mEnchanting.setSelfEnchanting(false);
        mEnchanting.setEnchanter(player);

        mPtr = player;

        startEditing ();
    }

    void EnchantingDialog::startSelfEnchanting(MWWorld::Ptr soulgem)
    {
        MWWorld::Ptr player = MWBase::Environment::get().getWorld()->getPlayer().getPlayer();

        mEnchanting.setSelfEnchanting(true);
        mEnchanting.setEnchanter(player);

        mPtr = player;

        startEditing();
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

        mEnchanting.setOldItem(item);
        mEnchanting.nextEnchantType();
        updateLabels();
    }

    void EnchantingDialog::onRemoveItem(MyGUI::Widget *sender)
    {
        while (mItemBox->getChildCount ())
            MyGUI::Gui::getInstance ().destroyWidget (mItemBox->getChildAt(0));
        mEnchanting.setOldItem(MWWorld::Ptr());
        updateLabels();
    }

    void EnchantingDialog::onItemCancel()
    {
        mItemSelectionDialog->setVisible(false);
    }

    void EnchantingDialog::onSoulSelected(MWWorld::Ptr item)
    {
        mItemSelectionDialog->setVisible(false);
        mEnchanting.setSoulGem(item);

        if(mEnchanting.getGemCharge()==0)
        {
            mWindowManager.messageBox ("#{sNotifyMessage32}");
            return;
        }

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
        updateLabels();
    }

    void EnchantingDialog::onRemoveSoul(MyGUI::Widget *sender)
    {
        while (mSoulBox->getChildCount ())
            MyGUI::Gui::getInstance ().destroyWidget (mSoulBox->getChildAt(0));
        mEnchanting.setSoulGem(MWWorld::Ptr());
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

    void EnchantingDialog::notifyEffectsChanged ()
    {
        mEffectList.mList = mEffects;
        mEnchanting.setEffect(mEffectList);
        updateLabels();
    }

    void EnchantingDialog::onTypeButtonClicked(MyGUI::Widget* sender)
    {
        mEnchanting.nextEnchantType();
        updateLabels();
    }

    void EnchantingDialog::onBuyButtonClicked(MyGUI::Widget* sender)
    {
        if (mEffects.size() <= 0)
        {
            mWindowManager.messageBox ("#{sNotifyMessage30}");
            return;
        }

        if (mName->getCaption ().empty())
        {
            mWindowManager.messageBox ("#{sNotifyMessage10}");
            return;
        }

        if (boost::lexical_cast<int>(mPrice->getCaption()) > mWindowManager.getInventoryWindow()->getPlayerGold())
        {
            mWindowManager.messageBox ("#{sNotifyMessage18}");
            return;
        }

        if (mEnchanting.soulEmpty())
        {
            mWindowManager.messageBox ("#{sNotifyMessage52}");
            return;
        }

        if (mEnchanting.itemEmpty())
        {
            mWindowManager.messageBox ("#{sNotifyMessage11}");
            return;
        }

        if (mEnchanting.getEnchantCost() > mEnchanting.getMaxEnchantValue())
        {
            mWindowManager.messageBox ("#{sNotifyMessage29}");
            return;
        }

        mEnchanting.setNewItemName(mName->getCaption());
        mEnchanting.setEffect(mEffectList);

        int result = mEnchanting.create();

        if(result==1)
            mWindowManager.messageBox ("#{sEnchantmentMenu12}");
        else
            mWindowManager.messageBox ("#{sNotifyMessage34}");

        mWindowManager.removeGuiMode (GM_Enchanting);
    }
}

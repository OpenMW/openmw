#include "enchantingdialog.hpp"

#include <boost/lexical_cast.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwworld/player.hpp"
#include "../mwworld/manualref.hpp"
#include "../mwworld/class.hpp"

#include "itemselection.hpp"
#include "container.hpp"
#include "inventorywindow.hpp"

#include "sortfilteritemmodel.hpp"

namespace MWGui
{


    EnchantingDialog::EnchantingDialog()
        : WindowBase("openmw_enchanting_dialog.layout")
        , EffectEditorBase()
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
        getWidget(mPriceText, "PriceTextLabel");

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
        std::stringstream enchantCost;
        enchantCost << std::setprecision(1) << std::fixed << mEnchanting.getEnchantPoints();
        mEnchantmentPoints->setCaption(enchantCost.str() + " / " + boost::lexical_cast<std::string>(mEnchanting.getMaxEnchantValue()));

        mCharge->setCaption(boost::lexical_cast<std::string>(mEnchanting.getGemCharge()));

        std::stringstream castCost;
        castCost << std::setprecision(1) << std::fixed << mEnchanting.getCastCost();
        mCastCost->setCaption(boost::lexical_cast<std::string>(castCost.str()));

        mPrice->setCaption(boost::lexical_cast<std::string>(mEnchanting.getEnchantPrice()));

        switch(mEnchanting.getCastStyle())
        {
            case ESM::Enchantment::CastOnce:
                mTypeButton->setCaption(MWBase::Environment::get().getWindowManager()->getGameSettingString("sItemCastOnce","Cast Once"));
                mAddEffectDialog.constantEffect=false;
                break;
            case ESM::Enchantment::WhenStrikes:
                mTypeButton->setCaption(MWBase::Environment::get().getWindowManager()->getGameSettingString("sItemCastWhenStrikes", "When Strikes"));
                mAddEffectDialog.constantEffect=false;
                break;
            case ESM::Enchantment::WhenUsed:
                mTypeButton->setCaption(MWBase::Environment::get().getWindowManager()->getGameSettingString("sItemCastWhenUsed", "When Used"));
                mAddEffectDialog.constantEffect=false;
                break;
            case ESM::Enchantment::ConstantEffect:
                mTypeButton->setCaption(MWBase::Environment::get().getWindowManager()->getGameSettingString("sItemCastConstant", "Cast Constant"));
                mAddEffectDialog.constantEffect=true;
                break;
        }
    }

    void EnchantingDialog::startEnchanting (MWWorld::Ptr actor)
    {
        mEnchanting.setSelfEnchanting(false);
        mEnchanting.setEnchanter(actor);

        mPtr = actor;

        startEditing ();
    }

    void EnchantingDialog::startSelfEnchanting(MWWorld::Ptr soulgem)
    {
        MWWorld::Ptr player = MWBase::Environment::get().getWorld()->getPlayer().getPlayer();

        mEnchanting.setSelfEnchanting(true);
        mEnchanting.setEnchanter(player);

        mPtr = player;
        startEditing();
        mEnchanting.setSoulGem(soulgem);

        MyGUI::ImageBox* image = mSoulBox->createWidget<MyGUI::ImageBox>("ImageBox", MyGUI::IntCoord(0, 0, 32, 32), MyGUI::Align::Default);
        std::string path = std::string("icons\\");
        path += MWWorld::Class::get(soulgem).getInventoryIcon(soulgem);
        int pos = path.rfind(".");
        path.erase(pos);
        path.append(".dds");
        image->setImageTexture (path);
        image->setUserString ("ToolTipType", "ItemPtr");
        image->setUserData(soulgem);
        image->eventMouseButtonClick += MyGUI::newDelegate(this, &EnchantingDialog::onRemoveSoul);

        mPrice->setVisible(false);
        mPriceText->setVisible(false);
        updateLabels();
    }

    void EnchantingDialog::onReferenceUnavailable ()
    {
        MWBase::Environment::get().getWindowManager()->removeGuiMode (GM_Dialogue);
        MWBase::Environment::get().getWindowManager()->removeGuiMode (GM_Enchanting);
    }

    void EnchantingDialog::onCancelButtonClicked(MyGUI::Widget* sender)
    {
        MWBase::Environment::get().getWindowManager()->removeGuiMode (GM_Enchanting);
    }

    void EnchantingDialog::onSelectItem(MyGUI::Widget *sender)
    {
        delete mItemSelectionDialog;
        mItemSelectionDialog = new ItemSelectionDialog("#{sEnchantItems}");
        mItemSelectionDialog->eventItemSelected += MyGUI::newDelegate(this, &EnchantingDialog::onItemSelected);
        mItemSelectionDialog->eventDialogCanceled += MyGUI::newDelegate(this, &EnchantingDialog::onItemCancel);
        mItemSelectionDialog->setVisible(true);
        mItemSelectionDialog->openContainer(MWBase::Environment::get().getWorld()->getPlayer().getPlayer());
        mItemSelectionDialog->setFilter(SortFilterItemModel::Filter_OnlyEnchantable);
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
        mEnchanting.nextCastStyle();
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
            MWBase::Environment::get().getWindowManager()->messageBox ("#{sNotifyMessage32}");
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
        mItemSelectionDialog = new ItemSelectionDialog("#{sSoulGemsWithSouls}");
        mItemSelectionDialog->eventItemSelected += MyGUI::newDelegate(this, &EnchantingDialog::onSoulSelected);
        mItemSelectionDialog->eventDialogCanceled += MyGUI::newDelegate(this, &EnchantingDialog::onSoulCancel);
        mItemSelectionDialog->setVisible(true);
        mItemSelectionDialog->openContainer(MWBase::Environment::get().getWorld()->getPlayer().getPlayer());
        mItemSelectionDialog->setFilter(SortFilterItemModel::Filter_OnlyChargedSoulstones);

        //MWBase::Environment::get().getWindowManager()->messageBox("#{sInventorySelectNoSoul}");
    }

    void EnchantingDialog::notifyEffectsChanged ()
    {
        mEffectList.mList = mEffects;
        mEnchanting.setEffect(mEffectList);
        updateLabels();
    }

    void EnchantingDialog::onTypeButtonClicked(MyGUI::Widget* sender)
    {
        mEnchanting.nextCastStyle();
        updateLabels();
    }

    void EnchantingDialog::onBuyButtonClicked(MyGUI::Widget* sender)
    {
        if (mEffects.size() <= 0)
        {
            MWBase::Environment::get().getWindowManager()->messageBox ("#{sNotifyMessage30}");
            return;
        }

        if (mName->getCaption ().empty())
        {
            MWBase::Environment::get().getWindowManager()->messageBox ("#{sNotifyMessage10}");
            return;
        }

        if (mEnchanting.soulEmpty())
        {
            MWBase::Environment::get().getWindowManager()->messageBox ("#{sNotifyMessage52}");
            return;
        }

        if (mEnchanting.itemEmpty())
        {
            MWBase::Environment::get().getWindowManager()->messageBox ("#{sNotifyMessage11}");
            return;
        }

        if (mEnchanting.getEnchantPoints() > mEnchanting.getMaxEnchantValue())
        {
            MWBase::Environment::get().getWindowManager()->messageBox ("#{sNotifyMessage29}");
            return;
        }

        mEnchanting.setNewItemName(mName->getCaption());
        mEnchanting.setEffect(mEffectList);

        if (mEnchanting.getEnchantPrice() > MWBase::Environment::get().getWindowManager()->getInventoryWindow()->getPlayerGold())
        {
            MWBase::Environment::get().getWindowManager()->messageBox ("#{sNotifyMessage18}");
            return;
        }

        int result = mEnchanting.create();

        if(result==1)
            MWBase::Environment::get().getWindowManager()->messageBox ("#{sEnchantmentMenu12}");
        else
            MWBase::Environment::get().getWindowManager()->messageBox ("#{sNotifyMessage34}");

        MWBase::Environment::get().getWindowManager()->removeGuiMode (GM_Enchanting);
    }
}

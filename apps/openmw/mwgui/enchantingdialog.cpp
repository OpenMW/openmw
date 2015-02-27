#include "enchantingdialog.hpp"

#include <iomanip>

#include <boost/lexical_cast.hpp>

#include <MyGUI_Button.h>
#include <MyGUI_ScrollView.h>

#include <components/esm/records.hpp>
#include <components/widgets/list.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwbase/soundmanager.hpp"
#include "../mwbase/dialoguemanager.hpp"
#include "../mwbase/mechanicsmanager.hpp"
#include "../mwworld/class.hpp"
#include "../mwworld/containerstore.hpp"
#include "../mwworld/esmstore.hpp"

#include "itemselection.hpp"
#include "itemwidget.hpp"

#include "sortfilteritemmodel.hpp"

namespace MWGui
{


    EnchantingDialog::EnchantingDialog()
        : WindowBase("openmw_enchanting_dialog.layout")
        , EffectEditorBase(EffectEditorBase::Enchanting)
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
    }

    void EnchantingDialog::setSoulGem(const MWWorld::Ptr &gem)
    {
        if (gem.isEmpty())
        {
            mSoulBox->setItem(MWWorld::Ptr());
            mSoulBox->clearUserStrings();
            mEnchanting.setSoulGem(MWWorld::Ptr());
        }
        else
        {
            mSoulBox->setItem(gem);
            mSoulBox->setUserString ("ToolTipType", "ItemPtr");
            mSoulBox->setUserData(gem);
            mEnchanting.setSoulGem(gem);
        }
    }

    void EnchantingDialog::setItem(const MWWorld::Ptr &item)
    {
        if (item.isEmpty())
        {
            mItemBox->setItem(MWWorld::Ptr());
            mItemBox->clearUserStrings();
            mEnchanting.setOldItem(MWWorld::Ptr());
        }
        else
        {
            mName->setCaption(item.getClass().getName(item));
            mItemBox->setItem(item);
            mItemBox->setUserString ("ToolTipType", "ItemPtr");
            mItemBox->setUserData(item);
            mEnchanting.setOldItem(item);
        }
    }

    void EnchantingDialog::exit()
    {
        MWBase::Environment::get().getWindowManager()->removeGuiMode (GM_Enchanting);
    }

    void EnchantingDialog::updateLabels()
    {
        std::stringstream enchantCost;
        enchantCost << std::setprecision(1) << std::fixed << mEnchanting.getEnchantPoints();
        mEnchantmentPoints->setCaption(enchantCost.str() + " / " + MyGUI::utility::toString(mEnchanting.getMaxEnchantValue()));

        mCharge->setCaption(MyGUI::utility::toString(mEnchanting.getGemCharge()));

        std::stringstream castCost;
        castCost << mEnchanting.getEffectiveCastCost();
        mCastCost->setCaption(castCost.str());

        mPrice->setCaption(MyGUI::utility::toString(mEnchanting.getEnchantPrice()));

        switch(mEnchanting.getCastStyle())
        {
            case ESM::Enchantment::CastOnce:
                mTypeButton->setCaption(MWBase::Environment::get().getWindowManager()->getGameSettingString("sItemCastOnce","Cast Once"));
                setConstantEffect(false);
                break;
            case ESM::Enchantment::WhenStrikes:
                mTypeButton->setCaption(MWBase::Environment::get().getWindowManager()->getGameSettingString("sItemCastWhenStrikes", "When Strikes"));
                setConstantEffect(false);
                break;
            case ESM::Enchantment::WhenUsed:
                mTypeButton->setCaption(MWBase::Environment::get().getWindowManager()->getGameSettingString("sItemCastWhenUsed", "When Used"));
                setConstantEffect(false);
                break;
            case ESM::Enchantment::ConstantEffect:
                mTypeButton->setCaption(MWBase::Environment::get().getWindowManager()->getGameSettingString("sItemCastConstant", "Cast Constant"));
                setConstantEffect(true);
                break;
        }
    }

    void EnchantingDialog::startEnchanting (MWWorld::Ptr actor)
    {
        mEnchanting.setSelfEnchanting(false);
        mEnchanting.setEnchanter(actor);

        mBuyButton->setCaptionWithReplacing("#{sBuy}");

        mPtr = actor;

        setSoulGem(MWWorld::Ptr());
        setItem(MWWorld::Ptr());

        startEditing ();
        mPrice->setVisible(true);
        mPriceText->setVisible(true);
        updateLabels();
    }

    void EnchantingDialog::startSelfEnchanting(MWWorld::Ptr soulgem)
    {
        MWWorld::Ptr player = MWBase::Environment::get().getWorld()->getPlayerPtr();

        mEnchanting.setSelfEnchanting(true);
        mEnchanting.setEnchanter(player);

        mBuyButton->setCaptionWithReplacing("#{sCreate}");

        mPtr = player;
        startEditing();

        setSoulGem(soulgem);
        setItem(MWWorld::Ptr());

        mPrice->setVisible(false);
        mPriceText->setVisible(false);
        updateLabels();
    }

    void EnchantingDialog::onReferenceUnavailable ()
    {
        MWBase::Environment::get().getWindowManager()->removeGuiMode (GM_Dialogue);
        MWBase::Environment::get().getWindowManager()->removeGuiMode (GM_Enchanting);
        resetReference();
    }

    void EnchantingDialog::resetReference()
    {
        ReferenceInterface::resetReference();
        setItem(MWWorld::Ptr());
        setSoulGem(MWWorld::Ptr());
        mPtr = MWWorld::Ptr();
        mEnchanting.setEnchanter(MWWorld::Ptr());
    }

    void EnchantingDialog::onCancelButtonClicked(MyGUI::Widget* sender)
    {
        exit();
    }

    void EnchantingDialog::onSelectItem(MyGUI::Widget *sender)
    {
        if (mEnchanting.getOldItem().isEmpty())
        {
            delete mItemSelectionDialog;
            mItemSelectionDialog = new ItemSelectionDialog("#{sEnchantItems}");
            mItemSelectionDialog->eventItemSelected += MyGUI::newDelegate(this, &EnchantingDialog::onItemSelected);
            mItemSelectionDialog->eventDialogCanceled += MyGUI::newDelegate(this, &EnchantingDialog::onItemCancel);
            mItemSelectionDialog->setVisible(true);
            mItemSelectionDialog->openContainer(MWBase::Environment::get().getWorld()->getPlayerPtr());
            mItemSelectionDialog->setFilter(SortFilterItemModel::Filter_OnlyEnchantable);
        }
        else
        {
            setItem(MWWorld::Ptr());
            updateLabels();
        }
    }

    void EnchantingDialog::onItemSelected(MWWorld::Ptr item)
    {
        mItemSelectionDialog->setVisible(false);

        setItem(item);
        MWBase::Environment::get().getSoundManager()->playSound(item.getClass().getDownSoundId(item), 1, 1);
        mEnchanting.nextCastStyle();
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

        setSoulGem(item);
        MWBase::Environment::get().getSoundManager()->playSound(item.getClass().getDownSoundId(item), 1, 1);
        updateLabels();
    }

    void EnchantingDialog::onSoulCancel()
    {
        mItemSelectionDialog->setVisible(false);
    }

    void EnchantingDialog::onSelectSoul(MyGUI::Widget *sender)
    {
        if (mEnchanting.getGem().isEmpty())
        {
            delete mItemSelectionDialog;
            mItemSelectionDialog = new ItemSelectionDialog("#{sSoulGemsWithSouls}");
            mItemSelectionDialog->eventItemSelected += MyGUI::newDelegate(this, &EnchantingDialog::onSoulSelected);
            mItemSelectionDialog->eventDialogCanceled += MyGUI::newDelegate(this, &EnchantingDialog::onSoulCancel);
            mItemSelectionDialog->setVisible(true);
            mItemSelectionDialog->openContainer(MWBase::Environment::get().getWorld()->getPlayerPtr());
            mItemSelectionDialog->setFilter(SortFilterItemModel::Filter_OnlyChargedSoulstones);

            //MWBase::Environment::get().getWindowManager()->messageBox("#{sInventorySelectNoSoul}");
        }
        else
        {
            setSoulGem(MWWorld::Ptr());
            updateLabels();
        }
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
        updateEffectsView();
    }

    void EnchantingDialog::onBuyButtonClicked(MyGUI::Widget* sender)
    {
        if (mEffects.size() <= 0)
        {
            MWBase::Environment::get().getWindowManager()->messageBox ("#{sEnchantmentMenu11}");
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

        MWWorld::Ptr player = MWBase::Environment::get().getWorld()->getPlayerPtr();
        int playerGold = player.getClass().getContainerStore(player).count(MWWorld::ContainerStore::sGoldId);
        if (mPtr != player && mEnchanting.getEnchantPrice() > playerGold)
        {
            MWBase::Environment::get().getWindowManager()->messageBox ("#{sNotifyMessage18}");
            return;
        }

        // check if the player is attempting to use a soulstone or item that was stolen from this actor
        if (mPtr != player)
        {
            for (int i=0; i<2; ++i)
            {
                MWWorld::Ptr item = (i == 0) ? mEnchanting.getOldItem() : mEnchanting.getGem();
                if (MWBase::Environment::get().getMechanicsManager()->isItemStolenFrom(item.getCellRef().getRefId(),
                                                                                       mPtr.getCellRef().getRefId()))
                {
                    std::string msg = MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>().find("sNotifyMessage49")->getString();
                    if (msg.find("%s") != std::string::npos)
                        msg.replace(msg.find("%s"), 2, item.getClass().getName(item));
                    MWBase::Environment::get().getWindowManager()->messageBox(msg);
                    MWBase::Environment::get().getMechanicsManager()->commitCrime(player, mPtr, MWBase::MechanicsManager::OT_Theft,
                                                                                  item.getClass().getValue(item), true);
                    MWBase::Environment::get().getWindowManager()->removeGuiMode (GM_Enchanting);
                    MWBase::Environment::get().getDialogueManager()->goodbyeSelected();
                    return;
                }
            }
        }

        int result = mEnchanting.create();

        if(result==1)
        {
            MWBase::Environment::get().getSoundManager()->playSound("enchant success", 1.f, 1.f);
            MWBase::Environment::get().getWindowManager()->messageBox ("#{sEnchantmentMenu12}");
        }
        else
        {
            MWBase::Environment::get().getSoundManager()->playSound("enchant fail", 1.f, 1.f);
            MWBase::Environment::get().getWindowManager()->messageBox ("#{sNotifyMessage34}");
        }

        MWBase::Environment::get().getWindowManager()->removeGuiMode (GM_Enchanting);
    }
}

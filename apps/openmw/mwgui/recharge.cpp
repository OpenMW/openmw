#include "recharge.hpp"

#include <MyGUI_ScrollView.h>

#include <components/widgets/box.hpp>

#include <components/esm3/loadcrea.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/windowmanager.hpp"

#include "../mwworld/class.hpp"
#include "../mwworld/esmstore.hpp"

#include "../mwmechanics/actorutil.hpp"
#include "../mwmechanics/recharge.hpp"

#include "inventoryitemmodel.hpp"
#include "itemchargeview.hpp"
#include "itemselection.hpp"
#include "itemwidget.hpp"
#include "sortfilteritemmodel.hpp"

namespace MWGui
{

    Recharge::Recharge()
        : WindowBase("openmw_recharge_dialog.layout")
    {
        getWidget(mBox, "Box");
        getWidget(mGemBox, "GemBox");
        getWidget(mGemIcon, "GemIcon");
        getWidget(mChargeLabel, "ChargeLabel");
        getWidget(mCancelButton, "CancelButton");

        mCancelButton->eventMouseButtonClick += MyGUI::newDelegate(this, &Recharge::onCancel);
        mBox->eventItemClicked += MyGUI::newDelegate(this, &Recharge::onItemClicked);

        mBox->setDisplayMode(ItemChargeView::DisplayMode_EnchantmentCharge);

        mGemIcon->eventMouseButtonClick += MyGUI::newDelegate(this, &Recharge::onSelectItem);

        mControllerButtons.mA = "#{OMWEngine:RechargeSelect}";
        mControllerButtons.mB = "#{Interface:Cancel}";
        mControllerButtons.mY = "#{sSoulGem}";
    }

    void Recharge::onOpen()
    {
        center();

        SortFilterItemModel* model
            = new SortFilterItemModel(std::make_unique<InventoryItemModel>(MWMechanics::getPlayer()));
        model->setFilter(SortFilterItemModel::Filter_OnlyRechargable);
        mBox->setModel(model);

        // Reset scrollbars
        mBox->resetScrollbars();
    }

    void Recharge::setPtr(const MWWorld::Ptr& item)
    {
        if (item.isEmpty() || !item.getClass().isItem(item))
            throw std::runtime_error("Invalid argument in Recharge::setPtr");

        mGemIcon->setItem(item);
        mGemIcon->setUserString("ToolTipType", "ItemPtr");
        mGemIcon->setUserData(MWWorld::Ptr(item));

        updateView();
    }

    void Recharge::updateView()
    {
        MWWorld::Ptr gem = *mGemIcon->getUserData<MWWorld::Ptr>();

        const ESM::RefId& soul = gem.getCellRef().getSoul();
        const ESM::Creature* creature = MWBase::Environment::get().getESMStore()->get<ESM::Creature>().find(soul);

        mChargeLabel->setCaptionWithReplacing("#{sCharges} " + MyGUI::utility::toString(creature->mData.mSoul));

        bool toolBoxVisible = gem.getCellRef().getCount() != 0;
        mGemBox->setVisible(toolBoxVisible);
        mGemBox->setUserString("Hidden", toolBoxVisible ? "false" : "true");

        if (!toolBoxVisible)
        {
            mGemIcon->setItem(MWWorld::Ptr());
            mGemIcon->clearUserStrings();
        }

        mBox->update();

        Gui::Box* box = dynamic_cast<Gui::Box*>(mMainWidget);
        if (box == nullptr)
            throw std::runtime_error("main widget must be a box");

        box->notifyChildrenSizeChanged();
        center();
    }

    void Recharge::onCancel(MyGUI::Widget* /*sender*/)
    {
        MWBase::Environment::get().getWindowManager()->removeGuiMode(GM_Recharge);
    }

    void Recharge::onSelectItem(MyGUI::Widget* /*sender*/)
    {
        mItemSelectionDialog = std::make_unique<ItemSelectionDialog>("#{sSoulGemsWithSouls}");
        mItemSelectionDialog->eventItemSelected += MyGUI::newDelegate(this, &Recharge::onItemSelected);
        mItemSelectionDialog->eventDialogCanceled += MyGUI::newDelegate(this, &Recharge::onItemCancel);
        mItemSelectionDialog->setVisible(true);
        mItemSelectionDialog->openContainer(MWMechanics::getPlayer());
        mItemSelectionDialog->setFilter(SortFilterItemModel::Filter_OnlyChargedSoulstones);
    }

    void Recharge::onItemSelected(MWWorld::Ptr item)
    {
        mItemSelectionDialog->setVisible(false);

        mGemIcon->setItem(item);
        mGemIcon->setUserString("ToolTipType", "ItemPtr");
        mGemIcon->setUserData(item);

        MWBase::Environment::get().getWindowManager()->playSound(item.getClass().getDownSoundId(item));
        updateView();
    }

    void Recharge::onItemCancel()
    {
        mItemSelectionDialog->setVisible(false);
    }

    void Recharge::onItemClicked(MyGUI::Widget* /*sender*/, const MWWorld::Ptr& item)
    {
        MWWorld::Ptr gem = *mGemIcon->getUserData<MWWorld::Ptr>();
        if (!MWMechanics::rechargeItem(item, gem))
            return;

        updateView();
    }

    bool Recharge::onControllerButtonEvent(const SDL_ControllerButtonEvent& arg)
    {
        if ((arg.button == SDL_CONTROLLER_BUTTON_A && !mGemBox->getVisible()) || arg.button == SDL_CONTROLLER_BUTTON_Y)
        {
            onSelectItem(mGemIcon);
            MWBase::Environment::get().getWindowManager()->playSound(ESM::RefId::stringRefId("Menu Click"));
        }
        else if (arg.button == SDL_CONTROLLER_BUTTON_B)
            onCancel(mCancelButton);
        else
            mBox->onControllerButton(arg.button);

        return true;
    }
}

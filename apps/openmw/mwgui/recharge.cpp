#include "recharge.hpp"

#include <MyGUI_ScrollView.h>
#include <MyGUI_Gui.h>

#include <components/widgets/box.hpp>

#include <components/misc/rng.hpp>

#include "../mwbase/world.hpp"
#include "../mwbase/environment.hpp"
#include "../mwbase/windowmanager.hpp"

#include "../mwworld/containerstore.hpp"
#include "../mwworld/class.hpp"
#include "../mwworld/esmstore.hpp"

#include "../mwmechanics/creaturestats.hpp"
#include "../mwmechanics/actorutil.hpp"
#include "../mwmechanics/recharge.hpp"

#include "itemwidget.hpp"
#include "itemchargeview.hpp"
#include "sortfilteritemmodel.hpp"
#include "inventoryitemmodel.hpp"

namespace MWGui
{

Recharge::Recharge()
    : WindowBase("openmw_recharge_dialog.layout")
    , mItemSelectionDialog(nullptr)
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
}

void Recharge::onOpen()
{
    center();

    SortFilterItemModel * model = new SortFilterItemModel(new InventoryItemModel(MWMechanics::getPlayer()));
    model->setFilter(SortFilterItemModel::Filter_OnlyRechargable);
    mBox->setModel(model);

    // Reset scrollbars
    mBox->resetScrollbars();
}

void Recharge::setPtr (const MWWorld::Ptr &item)
{
    mGemIcon->setItem(item);
    mGemIcon->setUserString("ToolTipType", "ItemPtr");
    mGemIcon->setUserData(MWWorld::Ptr(item));

    updateView();
}

void Recharge::updateView()
{
    MWWorld::Ptr gem = *mGemIcon->getUserData<MWWorld::Ptr>();

    std::string soul = gem.getCellRef().getSoul();
    const ESM::Creature *creature = MWBase::Environment::get().getWorld()->getStore().get<ESM::Creature>().find(soul);

    mChargeLabel->setCaptionWithReplacing("#{sCharges} " + MyGUI::utility::toString(creature->mData.mSoul));

    bool toolBoxVisible = (gem.getRefData().getCount() != 0);
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

void Recharge::onCancel(MyGUI::Widget *sender)
{
    MWBase::Environment::get().getWindowManager()->removeGuiMode(GM_Recharge);
}

void Recharge::onSelectItem(MyGUI::Widget *sender)
{
    delete mItemSelectionDialog;
    mItemSelectionDialog = new ItemSelectionDialog("#{sSoulGemsWithSouls}");
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
    mGemIcon->setUserString ("ToolTipType", "ItemPtr");
    mGemIcon->setUserData(item);

    MWBase::Environment::get().getWindowManager()->playSound(item.getClass().getDownSoundId(item));
    updateView();
}

void Recharge::onItemCancel()
{
    mItemSelectionDialog->setVisible(false);
}

void Recharge::onItemClicked(MyGUI::Widget *sender, const MWWorld::Ptr& item)
{
    MWWorld::Ptr gem = *mGemIcon->getUserData<MWWorld::Ptr>();
    if (!MWMechanics::rechargeItem(item, gem))
        return;

    updateView();
}

}

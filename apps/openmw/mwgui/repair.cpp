#include "repair.hpp"

#include <iomanip>

#include <MyGUI_ScrollView.h>

#include <components/widgets/box.hpp>

#include "../mwbase/world.hpp"
#include "../mwbase/environment.hpp"
#include "../mwbase/windowmanager.hpp"

#include "../mwmechanics/actorutil.hpp"

#include "../mwworld/containerstore.hpp"
#include "../mwworld/class.hpp"

#include "itemselection.hpp"
#include "itemwidget.hpp"
#include "itemchargeview.hpp"
#include "sortfilteritemmodel.hpp"
#include "inventoryitemmodel.hpp"

namespace MWGui
{

Repair::Repair()
    : WindowBase("openmw_repair.layout")
    , mItemSelectionDialog(nullptr)
{
    getWidget(mRepairBox, "RepairBox");
    getWidget(mToolBox, "ToolBox");
    getWidget(mToolIcon, "ToolIcon");
    getWidget(mUsesLabel, "UsesLabel");
    getWidget(mQualityLabel, "QualityLabel");
    getWidget(mCancelButton, "CancelButton");

    mCancelButton->eventMouseButtonClick += MyGUI::newDelegate(this, &Repair::onCancel);

    mRepairBox->eventItemClicked += MyGUI::newDelegate(this, &Repair::onRepairItem);
    mRepairBox->setDisplayMode(ItemChargeView::DisplayMode_Health);

    mToolIcon->eventMouseButtonClick += MyGUI::newDelegate(this, &Repair::onSelectItem);
}

void Repair::onOpen()
{
    center();

    SortFilterItemModel * model = new SortFilterItemModel(new InventoryItemModel(MWMechanics::getPlayer()));
    model->setFilter(SortFilterItemModel::Filter_OnlyRepairable);
    mRepairBox->setModel(model);

    // Reset scrollbars
    mRepairBox->resetScrollbars();
}

void Repair::setPtr(const MWWorld::Ptr &item)
{
    MWBase::Environment::get().getWindowManager()->playSound("Item Repair Up");

    mRepair.setTool(item);

    mToolIcon->setItem(item);
    mToolIcon->setUserString("ToolTipType", "ItemPtr");
    mToolIcon->setUserData(MWWorld::Ptr(item));

    updateRepairView();
}

void Repair::updateRepairView()
{
    MWWorld::LiveCellRef<ESM::Repair> *ref =
        mRepair.getTool().get<ESM::Repair>();

    int uses = mRepair.getTool().getClass().getItemHealth(mRepair.getTool());

    float quality = ref->mBase->mData.mQuality;

    mToolIcon->setUserData(mRepair.getTool());

    std::stringstream qualityStr;
    qualityStr << std::setprecision(3) << quality;

    mUsesLabel->setCaptionWithReplacing("#{sUses} " + MyGUI::utility::toString(uses));
    mQualityLabel->setCaptionWithReplacing("#{sQuality} " + qualityStr.str());

    bool toolBoxVisible = (mRepair.getTool().getRefData().getCount() != 0);
    mToolBox->setVisible(toolBoxVisible);
    mToolBox->setUserString("Hidden", toolBoxVisible ? "false" : "true");

    if (!toolBoxVisible)
    {
        mToolIcon->setItem(MWWorld::Ptr());
        mToolIcon->clearUserStrings();
    }

    mRepairBox->update();

    Gui::Box* box = dynamic_cast<Gui::Box*>(mMainWidget);
    if (box == nullptr)
        throw std::runtime_error("main widget must be a box");

    box->notifyChildrenSizeChanged();
    center();
}

void Repair::onSelectItem(MyGUI::Widget *sender)
{
    delete mItemSelectionDialog;
    mItemSelectionDialog = new ItemSelectionDialog("#{sRepair}");
    mItemSelectionDialog->eventItemSelected += MyGUI::newDelegate(this, &Repair::onItemSelected);
    mItemSelectionDialog->eventDialogCanceled += MyGUI::newDelegate(this, &Repair::onItemCancel);
    mItemSelectionDialog->setVisible(true);
    mItemSelectionDialog->openContainer(MWMechanics::getPlayer());
    mItemSelectionDialog->setFilter(SortFilterItemModel::Filter_OnlyRepairTools);
}

void Repair::onItemSelected(MWWorld::Ptr item)
{
    mItemSelectionDialog->setVisible(false);

    mToolIcon->setItem(item);
    mToolIcon->setUserString ("ToolTipType", "ItemPtr");
    mToolIcon->setUserData(item);

    mRepair.setTool(item);

    MWBase::Environment::get().getWindowManager()->playSound(item.getClass().getDownSoundId(item));
    updateRepairView();
}

void Repair::onItemCancel()
{
    mItemSelectionDialog->setVisible(false);
}

void Repair::onCancel(MyGUI::Widget* /*sender*/)
{
    MWBase::Environment::get().getWindowManager()->removeGuiMode(GM_Repair);
}

void Repair::onRepairItem(MyGUI::Widget* /*sender*/, const MWWorld::Ptr& ptr)
{
    if (!mRepair.getTool().getRefData().getCount())
        return;

    mRepair.repair(ptr);

    updateRepairView();
}

}

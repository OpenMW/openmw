#include "repair.hpp"

#include <iomanip>

#include <MyGUI_ScrollView.h>
#include <MyGUI_Gui.h>
#include <MyGUI_ItemBox.h>

#include <components/widgets/box.hpp>

#include "../mwbase/world.hpp"
#include "../mwbase/environment.hpp"
#include "../mwbase/windowmanager.hpp"

#include "../mwmechanics/actorutil.hpp"

#include "../mwworld/containerstore.hpp"
#include "../mwworld/class.hpp"

#include "widgets.hpp"

#include "itemwidget.hpp"
#include "itemchargeview.hpp"
#include "sortfilteritemmodel.hpp"
#include "inventoryitemmodel.hpp"

namespace MWGui
{

Repair::Repair()
    : WindowBase("openmw_repair.layout")
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
}

void Repair::open()
{
    center();

    SortFilterItemModel * model = new SortFilterItemModel(new InventoryItemModel(MWMechanics::getPlayer()));
    model->setFilter(SortFilterItemModel::Filter_OnlyRepairable);
    mRepairBox->setModel(model);

    // Reset scrollbars
    mRepairBox->resetScrollbars();
}

void Repair::exit()
{
    MWBase::Environment::get().getWindowManager()->removeGuiMode(GM_Repair);
}

void Repair::startRepairItem(const MWWorld::Ptr &item)
{
    mRepair.setTool(item);

    mToolIcon->setItem(item);
    mToolIcon->setUserString("ToolTipType", "ItemPtr");
    mToolIcon->setUserData(item);

    updateRepairView();
}

void Repair::updateRepairView()
{
    MWWorld::LiveCellRef<ESM::Repair> *ref =
        mRepair.getTool().get<ESM::Repair>();

    int uses = mRepair.getTool().getClass().getItemHealth(mRepair.getTool());

    float quality = ref->mBase->mData.mQuality;

    std::stringstream qualityStr;
    qualityStr << std::setprecision(3) << quality;

    mUsesLabel->setCaptionWithReplacing("#{sUses} " + MyGUI::utility::toString(uses));
    mQualityLabel->setCaptionWithReplacing("#{sQuality} " + qualityStr.str());

    bool toolBoxVisible = (mRepair.getTool().getRefData().getCount() != 0);
    mToolBox->setVisible(toolBoxVisible);
    mToolBox->setUserString("Hidden", toolBoxVisible ? "false" : "true");

    mRepairBox->update();

    Gui::Box* box = dynamic_cast<Gui::Box*>(mMainWidget);
    if (box == NULL)
        throw std::runtime_error("main widget must be a box");

    box->notifyChildrenSizeChanged();
    center();
}

void Repair::onCancel(MyGUI::Widget* /*sender*/)
{
    exit();
}

void Repair::onRepairItem(MyGUI::Widget* /*sender*/, const MWWorld::Ptr& ptr)
{
    if (!mRepair.getTool().getRefData().getCount())
        return;

    mRepair.repair(ptr);

    updateRepairView();
}

}

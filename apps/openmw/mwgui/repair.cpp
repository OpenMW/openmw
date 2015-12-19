#include "repair.hpp"

#include <iomanip>

#include <MyGUI_ScrollView.h>
#include <MyGUI_Gui.h>

#include "../mwbase/world.hpp"
#include "../mwbase/environment.hpp"
#include "../mwbase/windowmanager.hpp"

#include "../mwmechanics/actorutil.hpp"

#include "../mwworld/containerstore.hpp"
#include "../mwworld/class.hpp"

#include "widgets.hpp"

#include "itemwidget.hpp"

namespace MWGui
{

Repair::Repair()
    : WindowBase("openmw_repair.layout")
{
    getWidget(mRepairBox, "RepairBox");
    getWidget(mRepairView, "RepairView");
    getWidget(mToolBox, "ToolBox");
    getWidget(mToolIcon, "ToolIcon");
    getWidget(mUsesLabel, "UsesLabel");
    getWidget(mQualityLabel, "QualityLabel");
    getWidget(mCancelButton, "CancelButton");

    mCancelButton->eventMouseButtonClick += MyGUI::newDelegate(this, &Repair::onCancel);
}

void Repair::open()
{
    center();
    // Reset scrollbars
    mRepairView->setViewOffset(MyGUI::IntPoint(0, 0));
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

    bool toolBoxWasVisible = (mRepairBox->getPosition().top != mToolBox->getPosition().top);

    if (toolBoxVisible && !toolBoxWasVisible)
    {
        // shrink
        mRepairBox->setPosition(mRepairBox->getPosition() + MyGUI::IntPoint(0,mToolBox->getSize().height));
        mRepairBox->setSize(mRepairBox->getSize() - MyGUI::IntSize(0,mToolBox->getSize().height));
    }
    else if (!toolBoxVisible && toolBoxWasVisible)
    {
        // expand
        mRepairBox->setPosition(MyGUI::IntPoint (mRepairBox->getPosition().left, mToolBox->getPosition().top));
        mRepairBox->setSize(mRepairBox->getSize() + MyGUI::IntSize(0,mToolBox->getSize().height));
    }

    while (mRepairView->getChildCount())
        MyGUI::Gui::getInstance().destroyWidget(mRepairView->getChildAt(0));

    int currentY = 0;

    MWWorld::Ptr player = MWMechanics::getPlayer();
    MWWorld::ContainerStore& store = player.getClass().getContainerStore(player);
    int categories = MWWorld::ContainerStore::Type_Weapon | MWWorld::ContainerStore::Type_Armor;
    for (MWWorld::ContainerStoreIterator iter (store.begin(categories));
         iter!=store.end(); ++iter)
    {
        if (iter->getClass().hasItemHealth(*iter))
        {
            int maxDurability = iter->getClass().getItemMaxHealth(*iter);
            int durability = iter->getClass().getItemHealth(*iter);
            if (maxDurability == durability)
                continue;

            MyGUI::TextBox* text = mRepairView->createWidget<MyGUI::TextBox> (
                        "SandText", MyGUI::IntCoord(8, currentY, mRepairView->getWidth()-8, 18), MyGUI::Align::Default);
            text->setCaption(iter->getClass().getName(*iter));
            text->setNeedMouseFocus(false);
            currentY += 19;

            ItemWidget* icon = mRepairView->createWidget<ItemWidget> (
                        "MW_ItemIconSmall", MyGUI::IntCoord(16, currentY, 32, 32), MyGUI::Align::Default);
            icon->setItem(*iter);
            icon->setUserString("ToolTipType", "ItemPtr");
            icon->setUserData(*iter);
            icon->eventMouseButtonClick += MyGUI::newDelegate(this, &Repair::onRepairItem);
            icon->eventMouseWheel += MyGUI::newDelegate(this, &Repair::onMouseWheel);

            Widgets::MWDynamicStatPtr chargeWidget = mRepairView->createWidget<Widgets::MWDynamicStat>
                    ("MW_ChargeBar", MyGUI::IntCoord(72, currentY+2, 199, 20), MyGUI::Align::Default);
            chargeWidget->setValue(durability, maxDurability);
            chargeWidget->setNeedMouseFocus(false);

            currentY += 32 + 4;
        }
    }
    // Canvas size must be expressed with VScroll disabled, otherwise MyGUI would expand the scroll area when the scrollbar is hidden
    mRepairView->setVisibleVScroll(false);
    mRepairView->setCanvasSize (MyGUI::IntSize(mRepairView->getWidth(), std::max(mRepairView->getHeight(), currentY)));
    mRepairView->setVisibleVScroll(true);
}

void Repair::onCancel(MyGUI::Widget *sender)
{
    exit();
}

void Repair::onRepairItem(MyGUI::Widget *sender)
{
    if (!mRepair.getTool().getRefData().getCount())
        return;

    mRepair.repair(*sender->getUserData<MWWorld::Ptr>());

    updateRepairView();
}

void Repair::onMouseWheel(MyGUI::Widget* _sender, int _rel)
{
    if (mRepairView->getViewOffset().top + _rel*0.3f > 0)
        mRepairView->setViewOffset(MyGUI::IntPoint(0, 0));
    else
        mRepairView->setViewOffset(MyGUI::IntPoint(0, static_cast<int>(mRepairView->getViewOffset().top + _rel*0.3f)));
}

}

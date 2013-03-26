#include "repair.hpp"

#include <boost/lexical_cast.hpp>

#include "../mwbase/world.hpp"
#include "../mwbase/environment.hpp"
#include "../mwbase/windowmanager.hpp"

#include "../mwworld/player.hpp"
#include "../mwworld/containerstore.hpp"
#include "../mwworld/class.hpp"

#include "widgets.hpp"

namespace MWGui
{

Repair::Repair(MWBase::WindowManager &parWindowManager)
    : WindowBase("openmw_repair.layout", parWindowManager)
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
}

void Repair::startRepairItem(const MWWorld::Ptr &item)
{
    mRepair.setTool(item);

    std::string path = std::string("icons\\");
    path += MWWorld::Class::get(item).getInventoryIcon(item);
    int pos = path.rfind(".");
    path.erase(pos);
    path.append(".dds");
    mToolIcon->setImageTexture (path);
    mToolIcon->setUserString("ToolTipType", "ItemPtr");
    mToolIcon->setUserData(item);

    updateRepairView();
}

void Repair::updateRepairView()
{
    MWWorld::LiveCellRef<ESM::Repair> *ref =
        mRepair.getTool().get<ESM::Repair>();

    int uses = (mRepair.getTool().getCellRef().mCharge != -1) ? mRepair.getTool().getCellRef().mCharge : ref->mBase->mData.mUses;

    float quality = ref->mBase->mData.mQuality;

    std::stringstream qualityStr;
    qualityStr << std::setprecision(3) << quality;

    mUsesLabel->setCaptionWithReplacing("#{sUses} " + boost::lexical_cast<std::string>(uses));
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

    MWWorld::Ptr player = MWBase::Environment::get().getWorld()->getPlayer().getPlayer();
    MWWorld::ContainerStore& store = MWWorld::Class::get(player).getContainerStore(player);
    int categories = MWWorld::ContainerStore::Type_Weapon | MWWorld::ContainerStore::Type_Armor;
    for (MWWorld::ContainerStoreIterator iter (store.begin(categories));
         iter!=store.end(); ++iter)
    {
        if (MWWorld::Class::get(*iter).hasItemHealth(*iter))
        {
            int maxDurability = MWWorld::Class::get(*iter).getItemMaxHealth(*iter);
            int durability = (iter->getCellRef().mCharge == -1) ? maxDurability : iter->getCellRef().mCharge;
            if (maxDurability == durability)
                continue;

            MyGUI::TextBox* text = mRepairView->createWidget<MyGUI::TextBox> (
                        "SandText", MyGUI::IntCoord(8, currentY, mRepairView->getWidth()-8, 18), MyGUI::Align::Default);
            text->setCaption(MWWorld::Class::get(*iter).getName(*iter));
            text->setNeedMouseFocus(false);
            currentY += 19;

            MyGUI::ImageBox* icon = mRepairView->createWidget<MyGUI::ImageBox> (
                        "ImageBox", MyGUI::IntCoord(16, currentY, 32, 32), MyGUI::Align::Default);
            std::string path = std::string("icons\\");
            path += MWWorld::Class::get(*iter).getInventoryIcon(*iter);
            int pos = path.rfind(".");
            path.erase(pos);
            path.append(".dds");
            icon->setImageTexture (path);
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
    mRepairView->setCanvasSize (MyGUI::IntSize(mRepairView->getWidth(), std::max(mRepairView->getHeight(), currentY)));
}

void Repair::onCancel(MyGUI::Widget *sender)
{
    mWindowManager.removeGuiMode(GM_Repair);
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
    if (mRepairView->getViewOffset().top + _rel*0.3 > 0)
        mRepairView->setViewOffset(MyGUI::IntPoint(0, 0));
    else
        mRepairView->setViewOffset(MyGUI::IntPoint(0, mRepairView->getViewOffset().top + _rel*0.3));
}

}

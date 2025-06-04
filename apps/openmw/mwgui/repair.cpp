#include "repair.hpp"

#include <iomanip>

#include <MyGUI_ScrollView.h>

#include <components/esm3/loadrepa.hpp>
#include <components/widgets/box.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/windowmanager.hpp"

#include "../mwmechanics/actorutil.hpp"

#include "../mwworld/class.hpp"

#include "inventoryitemmodel.hpp"
#include "itemchargeview.hpp"
#include "itemselection.hpp"
#include "itemwidget.hpp"
#include "sortfilteritemmodel.hpp"

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

        mToolIcon->eventMouseButtonClick += MyGUI::newDelegate(this, &Repair::onSelectItem);

        mControllerButtons.a = "#{sRepair}";
        mControllerButtons.b = "#{sCancel}";
        mControllerButtons.y = "Tool";
    }

    void Repair::onOpen()
    {
        center();

        SortFilterItemModel* model
            = new SortFilterItemModel(std::make_unique<InventoryItemModel>(MWMechanics::getPlayer()));
        model->setFilter(SortFilterItemModel::Filter_OnlyRepairable);
        mRepairBox->setModel(model);
        mRepairBox->update();
        // Reset scrollbars
        mRepairBox->resetScrollbars();
    }

    void Repair::setPtr(const MWWorld::Ptr& item)
    {
        if (item.isEmpty() || !item.getClass().isItem(item))
            throw std::runtime_error("Invalid argument in Repair::setPtr");

        MWBase::Environment::get().getWindowManager()->playSound(ESM::RefId::stringRefId("Item Repair Up"));

        mRepair.setTool(item);

        mToolIcon->setItem(item);
        mToolIcon->setUserString("ToolTipType", "ItemPtr");
        mToolIcon->setUserData(MWWorld::Ptr(item));

        updateRepairView();
    }

    void Repair::updateRepairView()
    {
        MWWorld::LiveCellRef<ESM::Repair>* ref = mRepair.getTool().get<ESM::Repair>();

        int uses = mRepair.getTool().getClass().getItemHealth(mRepair.getTool());

        float quality = ref->mBase->mData.mQuality;

        mToolIcon->setUserData(mRepair.getTool());

        std::stringstream qualityStr;
        qualityStr << std::setprecision(3) << quality;

        mUsesLabel->setCaptionWithReplacing("#{sUses} " + MyGUI::utility::toString(uses));
        mQualityLabel->setCaptionWithReplacing("#{sQuality} " + qualityStr.str());

        bool toolBoxVisible = (mRepair.getTool().getCellRef().getCount() != 0);
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

    void Repair::onSelectItem(MyGUI::Widget* sender)
    {
        mItemSelectionDialog = std::make_unique<ItemSelectionDialog>("#{sRepair}");
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
        mToolIcon->setUserString("ToolTipType", "ItemPtr");
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
        if (!mRepair.getTool().getCellRef().getCount())
            return;

        mRepair.repair(ptr);

        updateRepairView();
    }

    bool Repair::onControllerButtonEvent(const SDL_ControllerButtonEvent& arg)
    {
        if ((arg.button == SDL_CONTROLLER_BUTTON_A && !mToolBox->getVisible())
            || arg.button == SDL_CONTROLLER_BUTTON_Y)
        {
            onSelectItem(mToolIcon);
            MWBase::Environment::get().getWindowManager()->playSound(ESM::RefId::stringRefId("Menu Click"));
        }
        else if (arg.button == SDL_CONTROLLER_BUTTON_B)
            onCancel(mCancelButton);
        else
            mRepairBox->onControllerButton(arg.button);

        return true;
    }
}

#include "recharge.hpp"

#include <boost/format.hpp>

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
#include "../mwmechanics/npcstats.hpp"
#include "../mwmechanics/actorutil.hpp"

#include "itemwidget.hpp"
#include "itemchargeview.hpp"
#include "sortfilteritemmodel.hpp"
#include "inventoryitemmodel.hpp"

namespace MWGui
{

Recharge::Recharge()
    : WindowBase("openmw_recharge_dialog.layout")
    , mItemSelectionDialog(NULL)
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
    if (box == NULL)
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

    if (!gem.getRefData().getCount())
        return;

    MWWorld::Ptr player = MWMechanics::getPlayer();
    MWMechanics::CreatureStats& stats = player.getClass().getCreatureStats(player);
    MWMechanics::NpcStats& npcStats = player.getClass().getNpcStats(player);

    float luckTerm = 0.1f * stats.getAttribute(ESM::Attribute::Luck).getModified();
    if (luckTerm < 1|| luckTerm > 10)
        luckTerm = 1;

    float intelligenceTerm = 0.2f * stats.getAttribute(ESM::Attribute::Intelligence).getModified();

    if (intelligenceTerm > 20)
        intelligenceTerm = 20;
    if (intelligenceTerm < 1)
        intelligenceTerm = 1;

    float x = (npcStats.getSkill(ESM::Skill::Enchant).getModified() + intelligenceTerm + luckTerm) * stats.getFatigueTerm();
    int roll = Misc::Rng::roll0to99();
    if (roll < x)
    {
        std::string soul = gem.getCellRef().getSoul();
        const ESM::Creature *creature = MWBase::Environment::get().getWorld()->getStore().get<ESM::Creature>().find(soul);

        float restored = creature->mData.mSoul * (roll / x);

        const ESM::Enchantment* enchantment = MWBase::Environment::get().getWorld()->getStore().get<ESM::Enchantment>().find(
                    item.getClass().getEnchantment(item));
        item.getCellRef().setEnchantmentCharge(
            std::min(item.getCellRef().getEnchantmentCharge() + restored, static_cast<float>(enchantment->mData.mCharge)));

        MWBase::Environment::get().getWindowManager()->playSound("Enchant Success");

        player.getClass().getContainerStore(player).restack(item);

        player.getClass().skillUsageSucceeded (player, ESM::Skill::Enchant, 0);
    }
    else
    {
        MWBase::Environment::get().getWindowManager()->playSound("Enchant Fail");
    }

    gem.getContainerStore()->remove(gem, 1, player);

    if (gem.getRefData().getCount() == 0)
    {
        std::string message = MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>().find("sNotifyMessage51")->getString();
        message = boost::str(boost::format(message) % gem.getClass().getName(gem));
        MWBase::Environment::get().getWindowManager()->messageBox(message);

        // special case: readd Azura's Star
        if (Misc::StringUtils::ciEqual(gem.get<ESM::Miscellaneous>()->mBase->mId, "Misc_SoulGem_Azura"))
            player.getClass().getContainerStore(player).add("Misc_SoulGem_Azura", 1, player);
    }

    updateView();
}

}

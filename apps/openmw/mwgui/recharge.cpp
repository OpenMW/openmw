#include "recharge.hpp"

#include <boost/format.hpp>

#include <MyGUI_ScrollView.h>
#include <MyGUI_Gui.h>

#include <openengine/misc/rng.hpp>

#include <components/esm/records.hpp>

#include "../mwbase/world.hpp"
#include "../mwbase/environment.hpp"
#include "../mwbase/windowmanager.hpp"

#include "../mwworld/containerstore.hpp"
#include "../mwworld/class.hpp"
#include "../mwworld/esmstore.hpp"

#include "../mwmechanics/creaturestats.hpp"
#include "../mwmechanics/npcstats.hpp"

#include "widgets.hpp"
#include "itemwidget.hpp"

namespace MWGui
{

Recharge::Recharge()
    : WindowBase("openmw_recharge_dialog.layout")
{
    getWidget(mBox, "Box");
    getWidget(mView, "View");
    getWidget(mGemBox, "GemBox");
    getWidget(mGemIcon, "GemIcon");
    getWidget(mChargeLabel, "ChargeLabel");
    getWidget(mCancelButton, "CancelButton");

    mCancelButton->eventMouseButtonClick += MyGUI::newDelegate(this, &Recharge::onCancel);

    setVisible(false);
}

void Recharge::open()
{
    center();
}

void Recharge::exit()
{
    MWBase::Environment::get().getWindowManager()->removeGuiMode(GM_Recharge);
}

void Recharge::start (const MWWorld::Ptr &item)
{
    mGemIcon->setItem(item);
    mGemIcon->setUserString("ToolTipType", "ItemPtr");
    mGemIcon->setUserData(item);

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

    bool toolBoxWasVisible = (mBox->getPosition().top != mGemBox->getPosition().top);

    if (toolBoxVisible && !toolBoxWasVisible)
    {
        // shrink
        mBox->setPosition(mBox->getPosition() + MyGUI::IntPoint(0, mGemBox->getSize().height));
        mBox->setSize(mBox->getSize() - MyGUI::IntSize(0,mGemBox->getSize().height));
    }
    else if (!toolBoxVisible && toolBoxWasVisible)
    {
        // expand
        mBox->setPosition(MyGUI::IntPoint (mBox->getPosition().left, mGemBox->getPosition().top));
        mBox->setSize(mBox->getSize() + MyGUI::IntSize(0,mGemBox->getSize().height));
    }

    while (mView->getChildCount())
        MyGUI::Gui::getInstance().destroyWidget(mView->getChildAt(0));

    int currentY = 0;

    MWWorld::Ptr player = MWBase::Environment::get().getWorld()->getPlayerPtr();
    MWWorld::ContainerStore& store = player.getClass().getContainerStore(player);
    for (MWWorld::ContainerStoreIterator iter (store.begin());
         iter!=store.end(); ++iter)
    {
        std::string enchantmentName = iter->getClass().getEnchantment(*iter);
        if (enchantmentName.empty())
            continue;
        const ESM::Enchantment* enchantment = MWBase::Environment::get().getWorld()->getStore().get<ESM::Enchantment>().find(enchantmentName);
        if (iter->getCellRef().getEnchantmentCharge() >= enchantment->mData.mCharge
                || iter->getCellRef().getEnchantmentCharge() == -1)
            continue;

        MyGUI::TextBox* text = mView->createWidget<MyGUI::TextBox> (
                    "SandText", MyGUI::IntCoord(8, currentY, mView->getWidth()-8, 18), MyGUI::Align::Default);
        text->setCaption(iter->getClass().getName(*iter));
        text->setNeedMouseFocus(false);
        currentY += 19;

        ItemWidget* icon = mView->createWidget<ItemWidget> (
                    "MW_ItemIconSmall", MyGUI::IntCoord(16, currentY, 32, 32), MyGUI::Align::Default);
        icon->setItem(*iter);
        icon->setUserString("ToolTipType", "ItemPtr");
        icon->setUserData(*iter);
        icon->eventMouseButtonClick += MyGUI::newDelegate(this, &Recharge::onItemClicked);
        icon->eventMouseWheel += MyGUI::newDelegate(this, &Recharge::onMouseWheel);

        Widgets::MWDynamicStatPtr chargeWidget = mView->createWidget<Widgets::MWDynamicStat>
                ("MW_ChargeBar", MyGUI::IntCoord(72, currentY+2, 199, 20), MyGUI::Align::Default);
        chargeWidget->setValue(static_cast<int>(iter->getCellRef().getEnchantmentCharge()), enchantment->mData.mCharge);
        chargeWidget->setNeedMouseFocus(false);

        currentY += 32 + 4;
    }

    // Canvas size must be expressed with VScroll disabled, otherwise MyGUI would expand the scroll area when the scrollbar is hidden
    mView->setVisibleVScroll(false);
    mView->setCanvasSize (MyGUI::IntSize(mView->getWidth(), std::max(mView->getHeight(), currentY)));
    mView->setVisibleVScroll(true);
}

void Recharge::onCancel(MyGUI::Widget *sender)
{
    exit();
}

void Recharge::onItemClicked(MyGUI::Widget *sender)
{
    MWWorld::Ptr gem = *mGemIcon->getUserData<MWWorld::Ptr>();

    if (!gem.getRefData().getCount())
        return;

    MWWorld::Ptr item = *sender->getUserData<MWWorld::Ptr>();

    MWWorld::Ptr player = MWBase::Environment::get().getWorld()->getPlayerPtr();
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
    int roll = OEngine::Misc::Rng::roll0to99();
    if (roll < x)
    {
        std::string soul = gem.getCellRef().getSoul();
        const ESM::Creature *creature = MWBase::Environment::get().getWorld()->getStore().get<ESM::Creature>().find(soul);

        float restored = creature->mData.mSoul * (roll / x);

        const ESM::Enchantment* enchantment = MWBase::Environment::get().getWorld()->getStore().get<ESM::Enchantment>().find(
                    item.getClass().getEnchantment(item));
        item.getCellRef().setEnchantmentCharge(
            std::min(item.getCellRef().getEnchantmentCharge() + restored, static_cast<float>(enchantment->mData.mCharge)));

        player.getClass().getContainerStore(player).restack(item);

        player.getClass().skillUsageSucceeded (player, ESM::Skill::Enchant, 0);
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

void Recharge::onMouseWheel(MyGUI::Widget* _sender, int _rel)
{
    if (mView->getViewOffset().top + _rel*0.3f > 0)
        mView->setViewOffset(MyGUI::IntPoint(0, 0));
    else
        mView->setViewOffset(MyGUI::IntPoint(0, static_cast<int>(mView->getViewOffset().top + _rel*0.3f)));
}

}

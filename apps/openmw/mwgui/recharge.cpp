#include "recharge.hpp"

#include <boost/lexical_cast.hpp>
#include <boost/format.hpp>

#include "../mwbase/world.hpp"
#include "../mwbase/environment.hpp"
#include "../mwbase/windowmanager.hpp"

#include "../mwworld/containerstore.hpp"
#include "../mwworld/class.hpp"

#include "../mwmechanics/creaturestats.hpp"
#include "../mwmechanics/npcstats.hpp"

#include "widgets.hpp"

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

void Recharge::start (const MWWorld::Ptr &item)
{
    std::string path = std::string("icons\\");
    path += MWWorld::Class::get(item).getInventoryIcon(item);
    int pos = path.rfind(".");
    path.erase(pos);
    path.append(".dds");
    mGemIcon->setImageTexture (path);
    mGemIcon->setUserString("ToolTipType", "ItemPtr");
    mGemIcon->setUserData(item);

    updateView();
}

void Recharge::updateView()
{
    MWWorld::Ptr gem = *mGemIcon->getUserData<MWWorld::Ptr>();

    std::string soul = gem.getCellRef().mSoul;
    const ESM::Creature *creature = MWBase::Environment::get().getWorld()->getStore().get<ESM::Creature>().find(soul);

    mChargeLabel->setCaptionWithReplacing("#{sCharges} " + boost::lexical_cast<std::string>(creature->mData.mSoul));

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
    MWWorld::ContainerStore& store = MWWorld::Class::get(player).getContainerStore(player);
    for (MWWorld::ContainerStoreIterator iter (store.begin());
         iter!=store.end(); ++iter)
    {
        std::string enchantmentName = iter->getClass().getEnchantment(*iter);
        if (enchantmentName.empty())
            continue;
        const ESM::Enchantment* enchantment = MWBase::Environment::get().getWorld()->getStore().get<ESM::Enchantment>().find(enchantmentName);
        if (iter->getCellRef().mEnchantmentCharge >= enchantment->mData.mCharge
                || iter->getCellRef().mEnchantmentCharge == -1)
            continue;

        MyGUI::TextBox* text = mView->createWidget<MyGUI::TextBox> (
                    "SandText", MyGUI::IntCoord(8, currentY, mView->getWidth()-8, 18), MyGUI::Align::Default);
        text->setCaption(MWWorld::Class::get(*iter).getName(*iter));
        text->setNeedMouseFocus(false);
        currentY += 19;

        MyGUI::ImageBox* icon = mView->createWidget<MyGUI::ImageBox> (
                    "ImageBox", MyGUI::IntCoord(16, currentY, 32, 32), MyGUI::Align::Default);
        std::string path = std::string("icons\\");
        path += MWWorld::Class::get(*iter).getInventoryIcon(*iter);
        int pos = path.rfind(".");
        path.erase(pos);
        path.append(".dds");
        icon->setImageTexture (path);
        icon->setUserString("ToolTipType", "ItemPtr");
        icon->setUserData(*iter);
        icon->eventMouseButtonClick += MyGUI::newDelegate(this, &Recharge::onItemClicked);
        icon->eventMouseWheel += MyGUI::newDelegate(this, &Recharge::onMouseWheel);

        Widgets::MWDynamicStatPtr chargeWidget = mView->createWidget<Widgets::MWDynamicStat>
                ("MW_ChargeBar", MyGUI::IntCoord(72, currentY+2, 199, 20), MyGUI::Align::Default);
        chargeWidget->setValue(iter->getCellRef().mEnchantmentCharge, enchantment->mData.mCharge);
        chargeWidget->setNeedMouseFocus(false);

        currentY += 32 + 4;
    }
    mView->setCanvasSize (MyGUI::IntSize(mView->getWidth(), std::max(mView->getHeight(), currentY)));
}

void Recharge::onCancel(MyGUI::Widget *sender)
{
    MWBase::Environment::get().getWindowManager()->removeGuiMode(GM_Recharge);
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

    float luckTerm = 0.1 * stats.getAttribute(ESM::Attribute::Luck).getModified();
    if (luckTerm < 1|| luckTerm > 10)
        luckTerm = 1;

    float intelligenceTerm = 0.2 * stats.getAttribute(ESM::Attribute::Intelligence).getModified();

    if (intelligenceTerm > 20)
        intelligenceTerm = 20;
    if (intelligenceTerm < 1)
        intelligenceTerm = 1;

    float x = (npcStats.getSkill(ESM::Skill::Enchant).getModified() + intelligenceTerm + luckTerm) * stats.getFatigueTerm();
    int roll = std::rand()/ (static_cast<double> (RAND_MAX) + 1) * 100; // [0, 99]
    if (roll < x)
    {
        std::string soul = gem.getCellRef().mSoul;
        const ESM::Creature *creature = MWBase::Environment::get().getWorld()->getStore().get<ESM::Creature>().find(soul);

        float restored = creature->mData.mSoul * (roll / x);

        const ESM::Enchantment* enchantment = MWBase::Environment::get().getWorld()->getStore().get<ESM::Enchantment>().find(
                    item.getClass().getEnchantment(item));
        item.getCellRef().mEnchantmentCharge =
            std::min(item.getCellRef().mEnchantmentCharge + restored, static_cast<float>(enchantment->mData.mCharge));

        player.getClass().skillUsageSucceeded (player, ESM::Skill::Enchant, 0);
    }

    gem.getContainerStore()->remove(gem, 1, player);

    if (gem.getRefData().getCount() == 0)
    {
        std::string message = MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>().find("sNotifyMessage51")->getString();
        message = boost::str(boost::format(message) % gem.getClass().getName(gem));
        MWBase::Environment::get().getWindowManager()->messageBox(message);
    }

    updateView();
}

void Recharge::onMouseWheel(MyGUI::Widget* _sender, int _rel)
{
    if (mView->getViewOffset().top + _rel*0.3 > 0)
        mView->setViewOffset(MyGUI::IntPoint(0, 0));
    else
        mView->setViewOffset(MyGUI::IntPoint(0, mView->getViewOffset().top + _rel*0.3));
}

}

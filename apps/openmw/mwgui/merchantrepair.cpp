#include "merchantrepair.hpp"

#include <components/esm/loadgmst.hpp>

#include <MyGUI_Button.h>
#include <MyGUI_ScrollView.h>
#include <MyGUI_Gui.h>

#include "../mwbase/world.hpp"
#include "../mwbase/environment.hpp"
#include "../mwbase/mechanicsmanager.hpp"
#include "../mwbase/windowmanager.hpp"

#include "../mwmechanics/creaturestats.hpp"
#include "../mwmechanics/actorutil.hpp"

#include "../mwworld/class.hpp"
#include "../mwworld/containerstore.hpp"
#include "../mwworld/esmstore.hpp"

namespace MWGui
{
MerchantRepair::MerchantRepair()
    : WindowBase("openmw_merchantrepair.layout")
{
    getWidget(mList, "RepairView");
    getWidget(mOkButton, "OkButton");
    getWidget(mGoldLabel, "PlayerGold");

    mOkButton->eventMouseButtonClick += MyGUI::newDelegate(this, &MerchantRepair::onOkButtonClick);
}

void MerchantRepair::setPtr(const MWWorld::Ptr &actor)
{
    mActor = actor;

    while (mList->getChildCount())
        MyGUI::Gui::getInstance().destroyWidget(mList->getChildAt(0));

    int lineHeight = MWBase::Environment::get().getWindowManager()->getFontHeight() + 2;
    int currentY = 0;

    MWWorld::Ptr player = MWMechanics::getPlayer();
    int playerGold = player.getClass().getContainerStore(player).count(MWWorld::ContainerStore::sGoldId);

    MWWorld::ContainerStore& store = player.getClass().getContainerStore(player);
    int categories = MWWorld::ContainerStore::Type_Weapon | MWWorld::ContainerStore::Type_Armor;
    for (MWWorld::ContainerStoreIterator iter (store.begin(categories)); iter!=store.end(); ++iter)
    {
        if (iter->getClass().hasItemHealth(*iter))
        {
            int maxDurability = iter->getClass().getItemMaxHealth(*iter);
            int durability = iter->getClass().getItemHealth(*iter);
            if (maxDurability == durability || maxDurability == 0)
                continue;

            int basePrice = iter->getClass().getValue(*iter);
            float fRepairMult = MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>()
                    .find("fRepairMult")->mValue.getFloat();

            float p = static_cast<float>(std::max(1, basePrice));
            float r = static_cast<float>(std::max(1, static_cast<int>(maxDurability / p)));

            int x = static_cast<int>((maxDurability - durability) / r);
            x = static_cast<int>(fRepairMult * x);
            x = std::max(1, x);

            int price = MWBase::Environment::get().getMechanicsManager()->getBarterOffer(mActor, x, true);

            std::string name = iter->getClass().getName(*iter)
                    + " - " + MyGUI::utility::toString(price)
                    + MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>()
                    .find("sgp")->mValue.getString();

            MyGUI::Button* button =
                mList->createWidget<MyGUI::Button>(price <= playerGold ? "SandTextButton" : "SandTextButtonDisabled", // can't use setEnabled since that removes tooltip
                    0,
                    currentY,
                    0,
                    lineHeight,
                    MyGUI::Align::Default
                );

            currentY += lineHeight;

            button->setUserString("Price", MyGUI::utility::toString(price));
            button->setUserData(MWWorld::Ptr(*iter));
            button->setCaptionWithReplacing(name);
            button->setSize(mList->getWidth(), lineHeight);
            button->eventMouseWheel += MyGUI::newDelegate(this, &MerchantRepair::onMouseWheel);
            button->setUserString("ToolTipType", "ItemPtr");
            button->eventMouseButtonClick += MyGUI::newDelegate(this, &MerchantRepair::onRepairButtonClick);
        }
    }
    // Canvas size must be expressed with VScroll disabled, otherwise MyGUI would expand the scroll area when the scrollbar is hidden
    mList->setVisibleVScroll(false);
    mList->setCanvasSize (MyGUI::IntSize(mList->getWidth(), std::max(mList->getHeight(), currentY)));
    mList->setVisibleVScroll(true);

    mGoldLabel->setCaptionWithReplacing("#{sGold}: "
        + MyGUI::utility::toString(playerGold));
}

void MerchantRepair::onMouseWheel(MyGUI::Widget* _sender, int _rel)
{
    if (mList->getViewOffset().top + _rel*0.3f > 0)
        mList->setViewOffset(MyGUI::IntPoint(0, 0));
    else
        mList->setViewOffset(MyGUI::IntPoint(0, static_cast<int>(mList->getViewOffset().top + _rel*0.3f)));
}

void MerchantRepair::onOpen()
{
    center();
    // Reset scrollbars
    mList->setViewOffset(MyGUI::IntPoint(0, 0));
}

void MerchantRepair::onRepairButtonClick(MyGUI::Widget *sender)
{
    MWWorld::Ptr player = MWMechanics::getPlayer();

    int price = MyGUI::utility::parseInt(sender->getUserString("Price"));
    if (price > player.getClass().getContainerStore(player).count(MWWorld::ContainerStore::sGoldId))
        return;

    // repair
    MWWorld::Ptr item = *sender->getUserData<MWWorld::Ptr>();
    item.getCellRef().setCharge(item.getClass().getItemMaxHealth(item));

    player.getClass().getContainerStore(player).restack(item);

    MWBase::Environment::get().getWindowManager()->playSound("Repair");

    player.getClass().getContainerStore(player).remove(MWWorld::ContainerStore::sGoldId, price, player);

    // add gold to NPC trading gold pool
    MWMechanics::CreatureStats& actorStats = mActor.getClass().getCreatureStats(mActor);
    actorStats.setGoldPool(actorStats.getGoldPool() + price);

    setPtr(mActor);
}

void MerchantRepair::onOkButtonClick(MyGUI::Widget *sender)
{
    MWBase::Environment::get().getWindowManager()->removeGuiMode(GM_MerchantRepair);
}

}

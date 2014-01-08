#include "merchantrepair.hpp"

#include <boost/lexical_cast.hpp>

#include "../mwbase/world.hpp"
#include "../mwbase/environment.hpp"
#include "../mwbase/mechanicsmanager.hpp"
#include "../mwbase/windowmanager.hpp"
#include "../mwbase/soundmanager.hpp"

#include "../mwworld/class.hpp"
#include "../mwworld/containerstore.hpp"

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

void MerchantRepair::startRepair(const MWWorld::Ptr &actor)
{
    mActor = actor;

    while (mList->getChildCount())
        MyGUI::Gui::getInstance().destroyWidget(mList->getChildAt(0));

    int currentY = 0;

    MWWorld::Ptr player = MWBase::Environment::get().getWorld()->getPlayerPtr();
    int playerGold = player.getClass().getContainerStore(player).count(MWWorld::ContainerStore::sGoldId);

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

            int basePrice = MWWorld::Class::get(*iter).getValue(*iter);
            float fRepairMult = MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>()
                    .find("fRepairMult")->getFloat();

            float p = std::max(1, basePrice);
            float r = std::max(1, static_cast<int>(maxDurability / p));

            int x = ((maxDurability - durability) / r);
            x = (fRepairMult * x);

            int price = MWBase::Environment::get().getMechanicsManager()->getBarterOffer(mActor, x, true);


            std::string name = MWWorld::Class::get(*iter).getName(*iter)
                    + " - " + boost::lexical_cast<std::string>(price)
                    + MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>()
                    .find("sgp")->getString();;


            MyGUI::Button* button =
                mList->createWidget<MyGUI::Button>("SandTextButton",
                    0,
                    currentY,
                    0,
                    18,
                    MyGUI::Align::Default
                );

            currentY += 18;

            button->setEnabled(price<=playerGold);
            button->setUserString("Price", boost::lexical_cast<std::string>(price));
            button->setUserData(*iter);
            button->setCaptionWithReplacing(name);
            button->setSize(button->getTextSize().width,18);
            button->eventMouseWheel += MyGUI::newDelegate(this, &MerchantRepair::onMouseWheel);
            button->setUserString("ToolTipType", "ItemPtr");
            button->eventMouseButtonClick += MyGUI::newDelegate(this, &MerchantRepair::onRepairButtonClick);
        }
    }
    mList->setCanvasSize (MyGUI::IntSize(mList->getWidth(), std::max(mList->getHeight(), currentY)));

    mGoldLabel->setCaptionWithReplacing("#{sGold}: "
        + boost::lexical_cast<std::string>(playerGold));
}

void MerchantRepair::onMouseWheel(MyGUI::Widget* _sender, int _rel)
{
    if (mList->getViewOffset().top + _rel*0.3 > 0)
        mList->setViewOffset(MyGUI::IntPoint(0, 0));
    else
        mList->setViewOffset(MyGUI::IntPoint(0, mList->getViewOffset().top + _rel*0.3));
}

void MerchantRepair::open()
{
    center();
}

void MerchantRepair::onRepairButtonClick(MyGUI::Widget *sender)
{
    // repair
    MWWorld::Ptr item = *sender->getUserData<MWWorld::Ptr>();
    item.getCellRef().mCharge = MWWorld::Class::get(item).getItemMaxHealth(item);

    MWBase::Environment::get().getSoundManager()->playSound("Repair",1,1);

    int price = boost::lexical_cast<int>(sender->getUserString("Price"));

    MWWorld::Ptr player = MWBase::Environment::get().getWorld()->getPlayerPtr();
    player.getClass().getContainerStore(player).remove(MWWorld::ContainerStore::sGoldId, price, player);

    startRepair(mActor);
}

void MerchantRepair::onOkButtonClick(MyGUI::Widget *sender)
{
    MWBase::Environment::get().getWindowManager()->removeGuiMode(GM_MerchantRepair);
}

}

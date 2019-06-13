#include "travelwindow.hpp"

#include <MyGUI_Button.h>
#include <MyGUI_ScrollView.h>
#include <MyGUI_Gui.h>

#include <components/settings/settings.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwbase/windowmanager.hpp"
#include "../mwbase/mechanicsmanager.hpp"

#include "../mwmechanics/creaturestats.hpp"
#include "../mwmechanics/actorutil.hpp"

#include "../mwworld/class.hpp"
#include "../mwworld/containerstore.hpp"
#include "../mwworld/actionteleport.hpp"
#include "../mwworld/esmstore.hpp"
#include "../mwworld/cellstore.hpp"

namespace MWGui
{
    TravelWindow::TravelWindow() :
        WindowBase("openmw_travel_window.layout")
        , mCurrentY(0)
    {
        getWidget(mCancelButton, "CancelButton");
        getWidget(mPlayerGold, "PlayerGold");
        getWidget(mSelect, "Select");
        getWidget(mDestinations, "Travel");
        getWidget(mDestinationsView, "DestinationsView");

        mCancelButton->eventMouseButtonClick += MyGUI::newDelegate(this, &TravelWindow::onCancelButtonClicked);

        mDestinations->setCoord(450/2-mDestinations->getTextSize().width/2,
                          mDestinations->getTop(),
                          mDestinations->getTextSize().width,
                          mDestinations->getHeight());
        mSelect->setCoord(8,
                          mSelect->getTop(),
                          mSelect->getTextSize().width,
                          mSelect->getHeight());
    }

    void TravelWindow::addDestination(const std::string& name, ESM::Position pos, bool interior)
    {
        int price;

        const MWWorld::Store<ESM::GameSetting> &gmst =
            MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>();

        MWWorld::Ptr player = MWBase::Environment::get().getWorld ()->getPlayerPtr();
        int playerGold = player.getClass().getContainerStore(player).count(MWWorld::ContainerStore::sGoldId);

        if (!mPtr.getCell()->isExterior())
        {
            price = gmst.find("fMagesGuildTravel")->mValue.getInteger();
        }
        else
        {
            ESM::Position PlayerPos = player.getRefData().getPosition();
            float d = sqrt(pow(pos.pos[0] - PlayerPos.pos[0], 2) + pow(pos.pos[1] - PlayerPos.pos[1], 2) + pow(pos.pos[2] - PlayerPos.pos[2], 2));
            price = static_cast<int>(d / gmst.find("fTravelMult")->mValue.getFloat());
        }

        price = MWBase::Environment::get().getMechanicsManager()->getBarterOffer(mPtr, price, true);

        // Add price for the travelling followers
        std::set<MWWorld::Ptr> followers;
        MWWorld::ActionTeleport::getFollowersToTeleport(player, followers);

        // Apply followers cost, unlike vanilla the first follower doesn't travel for free
        price *= 1 + static_cast<int>(followers.size());

        int lineHeight = MWBase::Environment::get().getWindowManager()->getFontHeight() + 2;

        MyGUI::Button* toAdd = mDestinationsView->createWidget<MyGUI::Button>("SandTextButton", 0, mCurrentY, 200, lineHeight, MyGUI::Align::Default);
        toAdd->setEnabled(price <= playerGold);
        mCurrentY += lineHeight;
        if(interior)
            toAdd->setUserString("interior","y");
        else
            toAdd->setUserString("interior","n");

        toAdd->setUserString("price", std::to_string(price));
        toAdd->setCaptionWithReplacing("#{sCell=" + name + "}   -   " + MyGUI::utility::toString(price)+"#{sgp}");
        toAdd->setSize(mDestinationsView->getWidth(),lineHeight);
        toAdd->eventMouseWheel += MyGUI::newDelegate(this, &TravelWindow::onMouseWheel);
        toAdd->setUserString("Destination", name);
        toAdd->setUserData(pos);
        toAdd->eventMouseButtonClick += MyGUI::newDelegate(this, &TravelWindow::onTravelButtonClick);
    }

    void TravelWindow::clearDestinations()
    {
        mDestinationsView->setViewOffset(MyGUI::IntPoint(0,0));
        mCurrentY = 0;
        while (mDestinationsView->getChildCount())
            MyGUI::Gui::getInstance().destroyWidget(mDestinationsView->getChildAt(0));
    }

    void TravelWindow::setPtr(const MWWorld::Ptr& actor)
    {
        center();
        mPtr = actor;
        clearDestinations();

        std::vector<ESM::Transport::Dest> transport;
        if (mPtr.getClass().isNpc())
            transport = mPtr.get<ESM::NPC>()->mBase->getTransport();
        else if (mPtr.getTypeName() == typeid(ESM::Creature).name())
            transport = mPtr.get<ESM::Creature>()->mBase->getTransport();

        for(unsigned int i = 0;i<transport.size();i++)
        {
            std::string cellname = transport[i].mCellName;
            bool interior = true;
            int x,y;
            MWBase::Environment::get().getWorld()->positionToIndex(transport[i].mPos.pos[0],
                                                                   transport[i].mPos.pos[1],x,y);
            if (cellname == "")
            {
                MWWorld::CellStore* cell = MWBase::Environment::get().getWorld()->getExterior(x,y);
                cellname = MWBase::Environment::get().getWorld()->getCellName(cell);
                interior = false;
            }
            addDestination(cellname,transport[i].mPos,interior);
        }

        updateLabels();
        // Canvas size must be expressed with VScroll disabled, otherwise MyGUI would expand the scroll area when the scrollbar is hidden
        mDestinationsView->setVisibleVScroll(false);
        mDestinationsView->setCanvasSize (MyGUI::IntSize(mDestinationsView->getWidth(), std::max(mDestinationsView->getHeight(), mCurrentY)));
        mDestinationsView->setVisibleVScroll(true);
    }

    void TravelWindow::onTravelButtonClick(MyGUI::Widget* _sender)
    {
        std::istringstream iss(_sender->getUserString("price"));
        int price;
        iss >> price;

        MWWorld::Ptr player = MWMechanics::getPlayer();
        int playerGold = player.getClass().getContainerStore(player).count(MWWorld::ContainerStore::sGoldId);

        if (playerGold<price)
            return;

        // Set "traveling" flag, so GetPCTraveling can detect teleportation.
        // We will reset this flag during next world update.
        MWBase::Environment::get().getWorld()->setPlayerTraveling(true);

        if (!mPtr.getCell()->isExterior())
            // Interior cell -> mages guild transport
            MWBase::Environment::get().getWindowManager()->playSound("mysticism cast");

        player.getClass().getContainerStore(player).remove(MWWorld::ContainerStore::sGoldId, price, player);

        // add gold to NPC trading gold pool
        MWMechanics::CreatureStats& npcStats = mPtr.getClass().getCreatureStats(mPtr);
        npcStats.setGoldPool(npcStats.getGoldPool() + price);

        MWBase::Environment::get().getWindowManager()->fadeScreenOut(1);
        ESM::Position pos = *_sender->getUserData<ESM::Position>();
        std::string cellname = _sender->getUserString("Destination");
        bool interior = _sender->getUserString("interior") == "y";
        if (mPtr.getCell()->isExterior())
        {
            ESM::Position playerPos = player.getRefData().getPosition();
            float d = (osg::Vec3f(pos.pos[0], pos.pos[1], 0) - osg::Vec3f(playerPos.pos[0], playerPos.pos[1], 0)).length();
            int hours = static_cast<int>(d /MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>().find("fTravelTimeMult")->mValue.getFloat());
            MWBase::Environment::get().getMechanicsManager ()->rest (hours, true);
            MWBase::Environment::get().getWorld()->advanceTime(hours);
        }

        MWBase::Environment::get().getWindowManager()->removeGuiMode(GM_Travel);
        MWBase::Environment::get().getWindowManager()->exitCurrentGuiMode();

        MWBase::Environment::get().getWindowManager()->fadeScreenOut(1);

        // Teleports any followers, too.
        MWWorld::ActionTeleport action(interior ? cellname : "", pos, true);
        action.execute(player);

        MWBase::Environment::get().getWindowManager()->fadeScreenOut(0);
        MWBase::Environment::get().getWindowManager()->fadeScreenIn(1);
    }

    void TravelWindow::onCancelButtonClicked(MyGUI::Widget* _sender)
    {
        MWBase::Environment::get().getWindowManager()->removeGuiMode(GM_Travel);
    }

    void TravelWindow::updateLabels()
    {
        MWWorld::Ptr player = MWBase::Environment::get().getWorld ()->getPlayerPtr();
        int playerGold = player.getClass().getContainerStore(player).count(MWWorld::ContainerStore::sGoldId);

        mPlayerGold->setCaptionWithReplacing("#{sGold}: " + MyGUI::utility::toString(playerGold));
        mPlayerGold->setCoord(8,
                              mPlayerGold->getTop(),
                              mPlayerGold->getTextSize().width,
                              mPlayerGold->getHeight());
    }

    void TravelWindow::onReferenceUnavailable()
    {
        MWBase::Environment::get().getWindowManager()->removeGuiMode(GM_Travel);
        MWBase::Environment::get().getWindowManager()->exitCurrentGuiMode();
    }

    void TravelWindow::onMouseWheel(MyGUI::Widget* _sender, int _rel)
    {
        if (mDestinationsView->getViewOffset().top + _rel*0.3f > 0)
            mDestinationsView->setViewOffset(MyGUI::IntPoint(0, 0));
        else
            mDestinationsView->setViewOffset(MyGUI::IntPoint(0, static_cast<int>(mDestinationsView->getViewOffset().top + _rel*0.3f)));
    }
}


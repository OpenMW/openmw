#include "travelwindow.hpp"

#include <MyGUI_Button.h>
#include <MyGUI_ScrollView.h>
#include <MyGUI_Gui.h>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwbase/windowmanager.hpp"
#include "../mwbase/mechanicsmanager.hpp"
#include "../mwbase/dialoguemanager.hpp"

#include "../mwmechanics/creaturestats.hpp"
#include "../mwmechanics/actorutil.hpp"

#include "../mwworld/class.hpp"
#include "../mwworld/containerstore.hpp"
#include "../mwworld/actionteleport.hpp"
#include "../mwworld/esmstore.hpp"
#include "../mwworld/cellstore.hpp"

namespace MWGui
{
    const int TravelWindow::sLineHeight = 18;

    TravelWindow::TravelWindow() :
        WindowBase("openmw_travel_window.layout")
        , mCurrentY(0)
        , mTravelTarget{}
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

    void TravelWindow::exit()
    {
        MWBase::Environment::get().getWindowManager()->removeGuiMode(GM_Travel);
    }

    void TravelWindow::addDestination(const std::string& name,ESM::Position pos,bool interior)
    {
        int price;

        const MWWorld::Store<ESM::GameSetting> &gmst =
            MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>();

        MWWorld::Ptr player = MWBase::Environment::get().getWorld ()->getPlayerPtr();
        int playerGold = player.getClass().getContainerStore(player).count(MWWorld::ContainerStore::sGoldId);

        if(interior)
        {
            price = gmst.find("fMagesGuildTravel")->getInt();
        }
        else
        {
            ESM::Position PlayerPos = player.getRefData().getPosition();
            float d = sqrt(pow(pos.pos[0] - PlayerPos.pos[0], 2) + pow(pos.pos[1] - PlayerPos.pos[1], 2) + pow(pos.pos[2] - PlayerPos.pos[2], 2));
            price = static_cast<int>(d / gmst.find("fTravelMult")->getFloat());
        }

        price = MWBase::Environment::get().getMechanicsManager()->getBarterOffer(mPtr, price, true);

        // Add price for the travelling followers
        std::set<MWWorld::Ptr> followers;
        MWWorld::ActionTeleport::getFollowersToTeleport(player, followers);

        // Apply followers cost, in vanilla one follower travels for free
        price *= std::max(1, static_cast<int>(followers.size()));

        MyGUI::Button* toAdd = mDestinationsView->createWidget<MyGUI::Button>("SandTextButton", 0, mCurrentY, 200, sLineHeight, MyGUI::Align::Default);
        toAdd->setEnabled(price <= playerGold);
        mCurrentY += sLineHeight;
        if(interior)
            toAdd->setUserString("interior","y");
        else
            toAdd->setUserString("interior","n");

        std::ostringstream oss;
        oss << price;
        toAdd->setUserString("price",oss.str());

        toAdd->setCaptionWithReplacing("#{sCell=" + name + "}   -   " + MyGUI::utility::toString(price)+"#{sgp}");
        toAdd->setSize(mDestinationsView->getWidth(),sLineHeight);
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

    void TravelWindow::startTravel(const MWWorld::Ptr& actor)
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

        if (!mPtr.getCell()->isExterior())
            // Interior cell -> mages guild transport
            MWBase::Environment::get().getWindowManager()->playSound("mysticism cast");

        player.getClass().getContainerStore(player).remove(MWWorld::ContainerStore::sGoldId, price, player);

        // add gold to NPC trading gold pool
        MWMechanics::CreatureStats& npcStats = mPtr.getClass().getCreatureStats(mPtr);
        npcStats.setGoldPool(npcStats.getGoldPool() + price);

        MWBase::Environment::get().getWindowManager()->fadeScreenOut(1);

        MWBase::Environment::get().getWorld()->setPlayerTraveling(true);

        mTravelTarget = MWBase::World::TravelTarget{ *_sender->getUserData<ESM::Position>(), _sender->getUserString("Destination"), _sender->getUserString("interior") == "y" };
    }

    void TravelWindow::onFrame()
    {
        if (!MWBase::Environment::get().getWorld()->isPlayerTraveling())
            return;
        
        MWBase::Environment::get().getWindowManager()->removeGuiMode(GM_Travel);
        MWBase::Environment::get().getDialogueManager()->goodbyeSelected();

        MWBase::Environment::get().getWorld()->travel(mTravelTarget);
    }

    void TravelWindow::onCancelButtonClicked(MyGUI::Widget* _sender)
    {
        exit();
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
        MWBase::Environment::get().getWindowManager()->removeGuiMode(GM_Dialogue);
    }

    void TravelWindow::onMouseWheel(MyGUI::Widget* _sender, int _rel)
    {
        if (mDestinationsView->getViewOffset().top + _rel*0.3f > 0)
            mDestinationsView->setViewOffset(MyGUI::IntPoint(0, 0));
        else
            mDestinationsView->setViewOffset(MyGUI::IntPoint(0, static_cast<int>(mDestinationsView->getViewOffset().top + _rel*0.3f)));
    }
}


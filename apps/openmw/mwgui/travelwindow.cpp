#include "travelwindow.hpp"

#include <MyGUI_Button.h>
#include <MyGUI_Gui.h>
#include <MyGUI_ScrollView.h>

#include <components/esm3/loadcrea.hpp>
#include <components/esm3/loadgmst.hpp>
#include <components/misc/strings/conversion.hpp>
#include <components/settings/values.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/mechanicsmanager.hpp"
#include "../mwbase/windowmanager.hpp"
#include "../mwbase/world.hpp"

#include "../mwworld/actionteleport.hpp"
#include "../mwworld/cellstore.hpp"
#include "../mwworld/class.hpp"
#include "../mwworld/containerstore.hpp"
#include "../mwworld/esmstore.hpp"
#include "../mwworld/store.hpp"
#include "../mwworld/worldmodel.hpp"

#include "../mwmechanics/actorutil.hpp"
#include "../mwmechanics/creaturestats.hpp"

namespace MWGui
{
    TravelWindow::TravelWindow()
        : WindowBase("openmw_travel_window.layout")
        , mCurrentY(0)
    {
        getWidget(mCancelButton, "CancelButton");
        getWidget(mPlayerGold, "PlayerGold");
        getWidget(mSelect, "Select");
        getWidget(mDestinations, "Travel");
        getWidget(mDestinationsView, "DestinationsView");

        mCancelButton->eventMouseButtonClick += MyGUI::newDelegate(this, &TravelWindow::onCancelButtonClicked);

        mDestinations->setCoord(450 / 2 - mDestinations->getTextSize().width / 2, mDestinations->getTop(),
            mDestinations->getTextSize().width, mDestinations->getHeight());
        mSelect->setCoord(8, mSelect->getTop(), mSelect->getTextSize().width, mSelect->getHeight());
    }

    void TravelWindow::addDestination(const ESM::RefId& name, const ESM::Position& pos, bool interior)
    {
        int price;

        const MWWorld::Store<ESM::GameSetting>& gmst
            = MWBase::Environment::get().getESMStore()->get<ESM::GameSetting>();

        MWWorld::Ptr player = MWBase::Environment::get().getWorld()->getPlayerPtr();
        int playerGold = player.getClass().getContainerStore(player).count(MWWorld::ContainerStore::sGoldId);

        if (!mPtr.getCell()->isExterior())
        {
            price = gmst.find("fMagesGuildTravel")->mValue.getInteger();
        }
        else
        {
            ESM::Position PlayerPos = player.getRefData().getPosition();
            float d = sqrt(pow(pos.pos[0] - PlayerPos.pos[0], 2) + pow(pos.pos[1] - PlayerPos.pos[1], 2)
                + pow(pos.pos[2] - PlayerPos.pos[2], 2));
            float fTravelMult = gmst.find("fTravelMult")->mValue.getFloat();
            if (fTravelMult != 0)
                price = static_cast<int>(d / fTravelMult);
            else
                price = static_cast<int>(d);
        }

        price = std::max(1, price);
        price = MWBase::Environment::get().getMechanicsManager()->getBarterOffer(mPtr, price, true);

        // Add price for the travelling followers
        std::set<MWWorld::Ptr> followers;
        MWWorld::ActionTeleport::getFollowers(player, followers, !interior);

        // Apply followers cost, unlike vanilla the first follower doesn't travel for free
        price *= 1 + static_cast<int>(followers.size());

        const int lineHeight = Settings::gui().mFontSize + 2;

        MyGUI::Button* toAdd = mDestinationsView->createWidget<MyGUI::Button>(
            "SandTextButton", 0, mCurrentY, 200, lineHeight, MyGUI::Align::Default);
        toAdd->setEnabled(price <= playerGold);
        mCurrentY += lineHeight;
        if (interior)
            toAdd->setUserString("interior", "y");
        else
            toAdd->setUserString("interior", "n");

        const std::string& nameString = name.getRefIdString();
        toAdd->setUserString("price", std::to_string(price));
        toAdd->setCaptionWithReplacing("#{sCell=" + nameString + "}  - " + MyGUI::utility::toString(price) + "#{sgp}");
        toAdd->setSize(mDestinationsView->getWidth(), lineHeight);
        toAdd->eventMouseWheel += MyGUI::newDelegate(this, &TravelWindow::onMouseWheel);
        toAdd->setUserString("Destination", nameString);
        toAdd->setUserData(pos);
        toAdd->eventMouseButtonClick += MyGUI::newDelegate(this, &TravelWindow::onTravelButtonClick);
    }

    void TravelWindow::clearDestinations()
    {
        mDestinationsView->setViewOffset(MyGUI::IntPoint(0, 0));
        mCurrentY = 0;
        while (mDestinationsView->getChildCount())
            MyGUI::Gui::getInstance().destroyWidget(mDestinationsView->getChildAt(0));
    }

    void TravelWindow::setPtr(const MWWorld::Ptr& actor)
    {
        if (actor.isEmpty() || !actor.getClass().isActor())
            throw std::runtime_error("Invalid argument in TravelWindow::setPtr");

        center();
        mPtr = actor;
        clearDestinations();

        std::vector<ESM::Transport::Dest> transport;
        if (mPtr.getClass().isNpc())
            transport = mPtr.get<ESM::NPC>()->mBase->getTransport();
        else if (mPtr.getType() == ESM::Creature::sRecordId)
            transport = mPtr.get<ESM::Creature>()->mBase->getTransport();

        for (const auto& dest : transport)
        {
            std::string_view cellname = dest.mCellName;
            bool interior = true;
            const ESM::ExteriorCellLocation cellIndex
                = ESM::positionToExteriorCellLocation(dest.mPos.pos[0], dest.mPos.pos[1]);
            if (cellname.empty())
            {
                MWWorld::CellStore& cell = MWBase::Environment::get().getWorldModel()->getExterior(cellIndex);
                cellname = MWBase::Environment::get().getWorld()->getCellName(&cell);
                interior = false;
            }
            addDestination(ESM::RefId::stringRefId(cellname), dest.mPos, interior);
        }

        updateLabels();
        // Canvas size must be expressed with VScroll disabled, otherwise MyGUI would expand the scroll area when the
        // scrollbar is hidden
        mDestinationsView->setVisibleVScroll(false);
        mDestinationsView->setCanvasSize(
            MyGUI::IntSize(mDestinationsView->getWidth(), std::max(mDestinationsView->getHeight(), mCurrentY)));
        mDestinationsView->setVisibleVScroll(true);
    }

    void TravelWindow::onTravelButtonClick(MyGUI::Widget* _sender)
    {
        const int price = Misc::StringUtils::toNumeric<int>(_sender->getUserString("price"), 0);

        MWWorld::Ptr player = MWMechanics::getPlayer();
        int playerGold = player.getClass().getContainerStore(player).count(MWWorld::ContainerStore::sGoldId);

        if (playerGold < price)
            return;

        // Set "traveling" flag, so GetPCTraveling can detect teleportation.
        // We will reset this flag during next world update.
        MWBase::Environment::get().getWorld()->setPlayerTraveling(true);

        if (!mPtr.getCell()->isExterior())
            // Interior cell -> mages guild transport
            MWBase::Environment::get().getWindowManager()->playSound(ESM::RefId::stringRefId("mysticism cast"));

        player.getClass().getContainerStore(player).remove(MWWorld::ContainerStore::sGoldId, price);

        // add gold to NPC trading gold pool
        MWMechanics::CreatureStats& npcStats = mPtr.getClass().getCreatureStats(mPtr);
        npcStats.setGoldPool(npcStats.getGoldPool() + price);

        MWBase::Environment::get().getWindowManager()->fadeScreenOut(1);
        ESM::Position pos = *_sender->getUserData<ESM::Position>();
        std::string_view cellname = _sender->getUserString("Destination");
        bool interior = _sender->getUserString("interior") == "y";
        if (mPtr.getCell()->isExterior())
        {
            ESM::Position playerPos = player.getRefData().getPosition();
            float d
                = (osg::Vec3f(pos.pos[0], pos.pos[1], 0) - osg::Vec3f(playerPos.pos[0], playerPos.pos[1], 0)).length();
            int hours = static_cast<int>(d
                / MWBase::Environment::get()
                      .getESMStore()
                      ->get<ESM::GameSetting>()
                      .find("fTravelTimeMult")
                      ->mValue.getFloat());
            MWBase::Environment::get().getMechanicsManager()->rest(hours, true);
            MWBase::Environment::get().getWorld()->advanceTime(hours);
        }

        MWBase::Environment::get().getWindowManager()->removeGuiMode(GM_Travel);
        MWBase::Environment::get().getWindowManager()->exitCurrentGuiMode();

        MWBase::Environment::get().getWindowManager()->fadeScreenOut(1);
        const ESM::ExteriorCellLocation posCell = ESM::positionToExteriorCellLocation(pos.pos[0], pos.pos[1]);
        ESM::RefId cellId = ESM::Cell::generateIdForCell(!interior, cellname, posCell.mX, posCell.mY);

        // Teleports any followers, too.
        MWWorld::ActionTeleport action(cellId, pos, true);
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
        MWWorld::Ptr player = MWBase::Environment::get().getWorld()->getPlayerPtr();
        int playerGold = player.getClass().getContainerStore(player).count(MWWorld::ContainerStore::sGoldId);

        mPlayerGold->setCaptionWithReplacing("#{sGold}: " + MyGUI::utility::toString(playerGold));
        mPlayerGold->setCoord(8, mPlayerGold->getTop(), mPlayerGold->getTextSize().width, mPlayerGold->getHeight());
    }

    void TravelWindow::onReferenceUnavailable()
    {
        MWBase::Environment::get().getWindowManager()->removeGuiMode(GM_Travel);
        MWBase::Environment::get().getWindowManager()->exitCurrentGuiMode();
    }

    void TravelWindow::onMouseWheel(MyGUI::Widget* _sender, int _rel)
    {
        if (mDestinationsView->getViewOffset().top + _rel * 0.3f > 0)
            mDestinationsView->setViewOffset(MyGUI::IntPoint(0, 0));
        else
            mDestinationsView->setViewOffset(
                MyGUI::IntPoint(0, static_cast<int>(mDestinationsView->getViewOffset().top + _rel * 0.3f)));
    }
}

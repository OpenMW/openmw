#include "travelwindow.hpp"

#include <algorithm>

#include <boost/lexical_cast.hpp>

#include <libs/openengine/ogre/fader.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwbase/soundmanager.hpp"
#include "../mwbase/windowmanager.hpp"
#include "../mwbase/mechanicsmanager.hpp"

#include "../mwworld/player.hpp"
#include "../mwworld/manualref.hpp"

#include "../mwmechanics/spells.hpp"
#include "../mwmechanics/creaturestats.hpp"

#include "inventorywindow.hpp"
#include "tradewindow.hpp"

namespace MWGui
{
    const int TravelWindow::sLineHeight = 18;

    TravelWindow::TravelWindow(MWBase::WindowManager& parWindowManager) :
        WindowBase("openmw_travel_window.layout", parWindowManager)
        , mCurrentY(0)
        , mLastPos(0)
    {
        setCoord(0, 0, 450, 300);

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

    void TravelWindow::addDestination(const std::string& travelId,ESM::Position pos,bool interior)
    {
        int price = 0;

        if(interior)
        {
            price = MWBase::Environment::get().getWorld()->getStore().gameSettings.find("fMagesGuildTravel")->getFloat();
        }
        else
        {
            MWWorld::Ptr player = MWBase::Environment::get().getWorld()->getPlayer().getPlayer();
            ESM::Position PlayerPos = player.getRefData().getPosition();
            float d = sqrt( pow(pos.pos[0] - PlayerPos.pos[0],2) + pow(pos.pos[1] - PlayerPos.pos[1],2) + pow(pos.pos[2] - PlayerPos.pos[2],2)   );
            price = d/MWBase::Environment::get().getWorld()->getStore().gameSettings.find("fTravelMult")->getFloat();
        }

        MyGUI::Button* toAdd = mDestinationsView->createWidget<MyGUI::Button>((price>mWindowManager.getInventoryWindow()->getPlayerGold()) ? "SandTextGreyedOut" : "SpellText", 0, mCurrentY, 200, sLineHeight, MyGUI::Align::Default);
        mCurrentY += sLineHeight;
        /// \todo price adjustment depending on merchantile skill
        if(interior)
            toAdd->setUserString("interior","y");
        else
            toAdd->setUserString("interior","n");

        std::ostringstream oss;
        oss << price;
        toAdd->setUserString("price",oss.str());

        toAdd->setCaptionWithReplacing(travelId+"   -   "+boost::lexical_cast<std::string>(price)+"#{sgp}");
        toAdd->setSize(toAdd->getTextSize().width,sLineHeight);
        toAdd->eventMouseWheel += MyGUI::newDelegate(this, &TravelWindow::onMouseWheel);
        toAdd->setUserString("Destination", travelId);
        toAdd->setUserData(pos);
        toAdd->eventMouseButtonClick += MyGUI::newDelegate(this, &TravelWindow::onTravelButtonClick);
        mDestinationsWidgetMap.insert(std::make_pair (toAdd, travelId));
    }

    void TravelWindow::clearDestinations()
    {
        mDestinationsView->setViewOffset(MyGUI::IntPoint(0,0));
        mCurrentY = 0;
        while (mDestinationsView->getChildCount())
            MyGUI::Gui::getInstance().destroyWidget(mDestinationsView->getChildAt(0));
        mDestinationsWidgetMap.clear();
    }

    void TravelWindow::startTravel(const MWWorld::Ptr& actor)
    {
        center();
        mPtr = actor;
        clearDestinations();

        MWWorld::Ptr player = MWBase::Environment::get().getWorld()->getPlayer().getPlayer();

        /*MWMechanics::Spells& playerSpells = MWWorld::Class::get (player).getCreatureStats (player).getSpells();
        MWMechanics::Spells& merchantSpells = MWWorld::Class::get (actor).getCreatureStats (actor).getSpells();
         
        for (MWMechanics::Spells::TIterator iter = merchantSpells.begin(); iter!=merchantSpells.end(); ++iter)
        {
            const ESM::Spell* spell = MWBase::Environment::get().getWorld()->getStore().spells.find (*iter);
            
            if (spell->data.type!=ESM::Spell::ST_Spell)
                continue; // don't try to sell diseases, curses or powers
            
            if (std::find (playerSpells.begin(), playerSpells.end(), *iter)!=playerSpells.end())
                continue; // we have that spell already
            
            addDestination (*iter);
        }*/

        for(unsigned int i = 0;i<mPtr.get<ESM::NPC>()->base->mTransport.size();i++)
        {
            std::string cellname = mPtr.get<ESM::NPC>()->base->mTransport[i].mCellName;
            bool interior = true;
            int x,y;
            MWBase::Environment::get().getWorld()->positionToIndex(mPtr.get<ESM::NPC>()->base->mTransport[i].mPos.pos[0],
                                                                   mPtr.get<ESM::NPC>()->base->mTransport[i].mPos.pos[1],x,y);
            if(cellname == "") {cellname = MWBase::Environment::get().getWorld()->getExterior(x,y)->cell->name; interior=  false;}
            addDestination(cellname,mPtr.get<ESM::NPC>()->base->mTransport[i].mPos,interior);
        }

        updateLabels();
        //mPtr.get<ESM::NPC>()->base->mTransport[0].
        mDestinationsView->setCanvasSize (MyGUI::IntSize(mDestinationsView->getWidth(), std::max(mDestinationsView->getHeight(), mCurrentY)));
    }

    void TravelWindow::onTravelButtonClick(MyGUI::Widget* _sender)
    {
        std::cout << "traveling to:" << _sender->getUserString("Destination");
        std::istringstream iss(_sender->getUserString("price"));
        int price;
        iss >> price;

        assert (mWindowManager.getInventoryWindow()->getPlayerGold()>=price);

        MWBase::Environment::get().getWorld ()->getFader ()->fadeOut(1);
        MWWorld::Ptr player = MWBase::Environment::get().getWorld()->getPlayer().getPlayer();
        ESM::Position pos = *_sender->getUserData<ESM::Position>();
        std::string cellname = _sender->getUserString("Destination");
        int x,y;
        bool interior = _sender->getUserString("interior") == "y";
        MWBase::Environment::get().getWorld()->positionToIndex(pos.pos[0],pos.pos[1],x,y);
        MWWorld::CellStore* cell;
        if(interior) cell = MWBase::Environment::get().getWorld()->getInterior(cellname);
        else
        {
            cell = MWBase::Environment::get().getWorld()->getExterior(x,y);
            ESM::Position PlayerPos = player.getRefData().getPosition();
            float d = sqrt( pow(pos.pos[0] - PlayerPos.pos[0],2) + pow(pos.pos[1] - PlayerPos.pos[1],2) + pow(pos.pos[2] - PlayerPos.pos[2],2)   );
            int time = int(d /MWBase::Environment::get().getWorld()->getStore().gameSettings.find("fTravelTimeMult")->getFloat());
            std::cout << time;
            for(int i = 0;i < time;i++)
            {
                MWBase::Environment::get().getMechanicsManager ()->restoreDynamicStats ();
            }
            MWBase::Environment::get().getWorld()->advanceTime(time);
        }
        MWBase::Environment::get().getWorld()->moveObject(player,*cell,pos.pos[0],pos.pos[1],pos.pos[2]);
        mWindowManager.removeGuiMode(GM_Travel);
        mWindowManager.removeGuiMode(GM_Dialogue);
        MWBase::Environment::get().getWorld ()->getFader ()->fadeOut(0);
        MWBase::Environment::get().getWorld ()->getFader ()->fadeIn(1);
    }

    void TravelWindow::onCancelButtonClicked(MyGUI::Widget* _sender)
    {
        mWindowManager.removeGuiMode(GM_Travel);
    }

    void TravelWindow::updateLabels()
    {
        mPlayerGold->setCaptionWithReplacing("#{sGold}: " + boost::lexical_cast<std::string>(mWindowManager.getInventoryWindow()->getPlayerGold()));
        mPlayerGold->setCoord(8,
                              mPlayerGold->getTop(),
                              mPlayerGold->getTextSize().width,
                              mPlayerGold->getHeight());
    }

    void TravelWindow::onReferenceUnavailable()
    {
        // remove both Spells and Dialogue (since you always trade with the NPC/creature that you have previously talked to)
        mWindowManager.removeGuiMode(GM_Travel);
        mWindowManager.removeGuiMode(GM_Dialogue);
    }

    void TravelWindow::onMouseWheel(MyGUI::Widget* _sender, int _rel)
    {
        if (mDestinationsView->getViewOffset().top + _rel*0.3 > 0)
            mDestinationsView->setViewOffset(MyGUI::IntPoint(0, 0));
        else
            mDestinationsView->setViewOffset(MyGUI::IntPoint(0, mDestinationsView->getViewOffset().top + _rel*0.3));
    }
}


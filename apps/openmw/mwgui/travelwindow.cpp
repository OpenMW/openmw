#include "travelwindow.hpp"

#include <algorithm>

#include <boost/lexical_cast.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwbase/soundmanager.hpp"
#include "../mwbase/windowmanager.hpp"

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

    void TravelWindow::addDestination(const std::string& travelId)
    {
        //std::cout << "travel to" << travelId;
        /*const ESM::Spell* spell = MWBase::Environment::get().getWorld()->getStore().spells.find(spellId);
        int price = spell->data.cost*MWBase::Environment::get().getWorld()->getStore().gameSettings.find("fSpellValueMult")->getFloat();*/
        int price = 0;
        MyGUI::Button* toAdd = mDestinationsView->createWidget<MyGUI::Button>((price>mWindowManager.getInventoryWindow()->getPlayerGold()) ? "SandTextGreyedOut" : "SpellText", 0, mCurrentY, 200, sLineHeight, MyGUI::Align::Default);
        mCurrentY += sLineHeight;
        /// \todo price adjustment depending on merchantile skill
        toAdd->setUserData(price);
        toAdd->setCaptionWithReplacing(travelId+"   -   "+boost::lexical_cast<std::string>(price)+"#{sgp}");
        toAdd->setSize(toAdd->getTextSize().width,sLineHeight);
        toAdd->eventMouseWheel += MyGUI::newDelegate(this, &TravelWindow::onMouseWheel);
        //toAdd->setUserString("ToolTipType", "Spell");
        toAdd->setUserString("Destination", travelId);
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

        for(int i = 0;i<mPtr.get<ESM::NPC>()->base->mTransport.size();i++)
        {
            std::string cellname = mPtr.get<ESM::NPC>()->base->mTransport[i].mCellName;
            int x,y;
            MWBase::Environment::get().getWorld()->positionToIndex(mPtr.get<ESM::NPC>()->base->mTransport[i].mPos.pos[0],
                                                                   mPtr.get<ESM::NPC>()->base->mTransport[i].mPos.pos[1],x,y);
            if(cellname == "") cellname = MWBase::Environment::get().getWorld()->getExterior(x,y)->cell->name;
            addDestination(cellname);
        }

        updateLabels();
        //mPtr.get<ESM::NPC>()->base->mTransport[0].
        mDestinationsView->setCanvasSize (MyGUI::IntSize(mDestinationsView->getWidth(), std::max(mDestinationsView->getHeight(), mCurrentY)));
    }

    void TravelWindow::onTravelButtonClick(MyGUI::Widget* _sender)
    {
        std::cout << "traveling to:" << _sender->getUserString("Destination");
        /*int price = *_sender->getUserData<int>();

        if (mWindowManager.getInventoryWindow()->getPlayerGold()>=price)
        {
            MWWorld::Ptr player = MWBase::Environment::get().getWorld()->getPlayer().getPlayer();
            MWMechanics::CreatureStats& stats = MWWorld::Class::get(player).getCreatureStats(player);
            MWMechanics::Spells& spells = stats.getSpells();
            spells.add (mSpellsWidgetMap.find(_sender)->second);
            mWindowManager.getTradeWindow()->addOrRemoveGold(-price);
            startSpellBuying(mPtr);

            MWBase::Environment::get().getSoundManager()->playSound ("Item Gold Up", 1.0, 1.0);
        }*/
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


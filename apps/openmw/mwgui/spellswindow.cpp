#include "spellswindow.hpp"

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
    const int SpellsWindow::sLineHeight = 18;

    SpellsWindow::SpellsWindow(MWBase::WindowManager& parWindowManager) :
        WindowBase("openmw_spells_window.layout", parWindowManager)
        , ContainerBase(NULL) // no drag&drop
        , mSpellsWidgetMap()
        , mCurrentY(0)
        , mLastPos(0)
        , mSpellsWidgets()
        , mSpellsPriceMap()
    {
        setCoord(0, 0, 450, 300);
        center();

        getWidget(mCancelButton, "CancelButton");
        getWidget(mPlayerGold, "PlayerGold");
        getWidget(mSelect, "Select");
        getWidget(mSpells, "Spells");
        getWidget(mSpellsBoxWidget, "SpellsBox");
        getWidget(mSpellsClientWidget, "SpellsClient");
        getWidget(mSpellsScrollerWidget, "SpellsScroller");

        mSpellsClientWidget->eventMouseWheel += MyGUI::newDelegate(this, &SpellsWindow::onMouseWheel);

        mSpellsScrollerWidget->eventScrollChangePosition += MyGUI::newDelegate(this, &SpellsWindow::onScrollChangePosition);

        mCancelButton->eventMouseButtonClick += MyGUI::newDelegate(this, &SpellsWindow::onCancelButtonClicked);

        int cancelButtonWidth = mCancelButton->getTextSize().width + 24;
        mCancelButton->setCoord(430-cancelButtonWidth,
                                mCancelButton->getTop(),
                                cancelButtonWidth,
                                mCancelButton->getHeight());
        mSpells->setCoord(450/2-mSpells->getTextSize().width/2,
                          mSpells->getTop(),
                          mSpells->getTextSize().width,
                          mSpells->getHeight());
        mSelect->setCoord(8,
                          mSelect->getTop(),
                          mSelect->getTextSize().width,
                          mSelect->getHeight());

        static_cast<MyGUI::Window*>(mMainWidget)->eventWindowChangeCoord += MyGUI::newDelegate(this, &SpellsWindow::onWindowResize);
    }

    void SpellsWindow::addSpell(std::string spellID)
    {
        MyGUI::Button* toAdd;

        toAdd = mSpellsClientWidget->createWidget<MyGUI::Button>("SandText", 0, mCurrentY, 200, sLineHeight, MyGUI::Align::Default);
        mCurrentY += sLineHeight;
        const ESM::Spell* spell = MWBase::Environment::get().getWorld()->getStore().spells.find(spellID);
        /// \todo price adjustment depending on merchantile skill
        int price = spell->data.cost*MWBase::Environment::get().getWorld()->getStore().gameSettings.search("fSpellValueMult")->f;
        mSpellsPriceMap.insert(std::pair<const ESM::Spell*, int>(spell,price));
        if (price>mWindowManager.getInventoryWindow()->getPlayerGold())
            toAdd->setCaption("#A3997B" + spell->name+"   -   "+boost::lexical_cast<std::string>(price)+MWBase::Environment::get().getWorld()->getStore().gameSettings.search("sgp")->str);
        else
            toAdd->setCaption(spell->name+"   -   "+boost::lexical_cast<std::string>(price)+MWBase::Environment::get().getWorld()->getStore().gameSettings.search("sgp")->str);
        toAdd->setSize(toAdd->getTextSize().width,sLineHeight);
        toAdd->eventMouseWheel += MyGUI::newDelegate(this, &SpellsWindow::onMouseWheel);
        toAdd->setUserString("ToolTipType", "Spell");
        toAdd->setUserString("Spell", spellID);
        toAdd->eventMouseButtonClick += MyGUI::newDelegate(this, &SpellsWindow::onSpellButtonClick);
        toAdd->eventMouseSetFocus += MyGUI::newDelegate(this, &SpellsWindow::onFocus);
        toAdd->eventMouseLostFocus += MyGUI::newDelegate(this, &SpellsWindow::onFocusLost);
        mSpellsWidgets.push_back(toAdd);
        mSpellsWidgetMap.insert(std::pair<MyGUI::Widget*, const ESM::Spell*>(toAdd,spell));
    }

    void SpellsWindow::onFocusLost(MyGUI::Widget* _sender, MyGUI::Widget* _old)
    {
        updateLabels();
    }

    void SpellsWindow::onFocus(MyGUI::Widget* _sender, MyGUI::Widget* _old)
    {
        updateLabels();
        MyGUI::Button* toUpdate;
        toUpdate = (MyGUI::Button*) _sender;
        const ESM::Spell* spell = mSpellsWidgetMap.find(toUpdate)->second;
        int price = mSpellsPriceMap.find(spell)->second;
        if (price>mWindowManager.getInventoryWindow()->getPlayerGold())
            toUpdate->setCaption("#A3997B" + spell->name+"   -   "+boost::lexical_cast<std::string>(price)+MWBase::Environment::get().getWorld()->getStore().gameSettings.search("sgp")->str);
        else
            toUpdate->setCaption("#D8C09A" + spell->name+"   -   "+boost::lexical_cast<std::string>(price)+MWBase::Environment::get().getWorld()->getStore().gameSettings.search("sgp")->str);
    }

    void SpellsWindow::clearSpells()
    {
        mSpellsScrollerWidget->setScrollPosition(0);
        onScrollChangePosition(mSpellsScrollerWidget, mSpellsScrollerWidget->getScrollPosition());
        mCurrentY = 0;
        mSpellsWidgets.clear();
        while (mSpellsClientWidget->getChildCount()!=0)
        {
            MyGUI::Widget* toRemove;
            toRemove = mSpellsClientWidget->getChildAt(0);
            mSpellsClientWidget->_destroyChildWidget(toRemove);
        }
        mSpellsWidgetMap.clear();
        mSpellsPriceMap.clear();
    }

    void SpellsWindow::startSpells(MWWorld::Ptr actor)
    {
        clearSpells();

        if (actor.getTypeName() == typeid(ESM::NPC).name())
        {
            MWWorld::Ptr player = MWBase::Environment::get().getWorld()->getPlayer().getPlayer();
            MWMechanics::CreatureStats& stats = MWWorld::Class::get(player).getCreatureStats(player);
            MWMechanics::Spells& playerSpells = stats.getSpells();
            std::vector<std::string> spellList = actor.get<ESM::NPC>()->base->spells.list;
            for (std::vector<std::string>::const_iterator it = spellList.begin(); it != spellList.end(); ++it)
            {
                bool alreadyHave = false;
                for (std::vector<std::string>::const_iterator it2 = playerSpells.begin(); it2 != playerSpells.end(); ++it2)
                {
                    std::string spellname1 = MWBase::Environment::get().getWorld()->getStore().spells.find(*it)->name;
                    std::string spellname2 = MWBase::Environment::get().getWorld()->getStore().spells.find(*it2)->name;
                    if (spellname1.compare(spellname2)==0)
                        alreadyHave = true;
                }
                if (alreadyHave==false)
                    addSpell(*it);
            }
        }
        updateLabels();
        updateScroller();
    }

    void SpellsWindow::onSpellButtonClick(MyGUI::Widget* _sender)
    {
        const ESM::Spell* spell = mSpellsWidgetMap.find(_sender)->second;
        int price = mSpellsPriceMap.find(spell)->second;

        if (mWindowManager.getInventoryWindow()->getPlayerGold()>=price)
        {
            MWWorld::Ptr player = MWBase::Environment::get().getWorld()->getPlayer().getPlayer();
            MWMechanics::CreatureStats& stats = MWWorld::Class::get(player).getCreatureStats(player);
            MWMechanics::Spells& spells = stats.getSpells();
            spells.add(spell->name);
            mWindowManager.getTradeWindow()->addOrRemoveGold(-price);
            mSpellsWidgetMap.erase(_sender);
            mSpellsPriceMap.erase(spell);
            for (std::vector<MyGUI::WidgetPtr>::iterator it = mSpellsWidgets.begin(); it != mSpellsWidgets.end(); ++it)
            {
                if (*it==_sender)
                {
                    mSpellsWidgets.erase(it);
                    break;
                }
            }
            mSpellsClientWidget->_destroyChildWidget(_sender);
            unsigned int i;
            mSpellsScrollerWidget->setScrollPosition(0);
            onScrollChangePosition(mSpellsScrollerWidget, mSpellsScrollerWidget->getScrollPosition());
            mCurrentY = 0;
            for (i=0;i<mSpellsClientWidget->getChildCount();i++)
            {
                MyGUI::Widget* toMove;
                toMove = mSpellsClientWidget->getChildAt(i);
                toMove->setPosition(0,mCurrentY);
                mCurrentY+=sLineHeight;
            }
        }
        else
        {
            
        }
        updateLabels();
        updateScroller();
    }

    void SpellsWindow::onWindowResize(MyGUI::Window* _sender)
    {
      
    }

    void SpellsWindow::onCancelButtonClicked(MyGUI::Widget* _sender)
    {
        mWindowManager.removeGuiMode(GM_Spells);
    }

    void SpellsWindow::updateLabels()
    {
        mPlayerGold->setCaption(MWBase::Environment::get().getWorld()->getStore().gameSettings.search("sGold")->str
            + ": " + boost::lexical_cast<std::string>(mWindowManager.getInventoryWindow()->getPlayerGold()));
        mPlayerGold->setCoord(8,
                              mPlayerGold->getTop(),
                              mPlayerGold->getTextSize().width,
                              mPlayerGold->getHeight());
        unsigned int i;
        for (i=0;i<mSpellsClientWidget->getChildCount();i++)
        {
            MyGUI::Button* toUpdate;
            toUpdate = (MyGUI::Button*) mSpellsClientWidget->getChildAt(i);
            const ESM::Spell* spell = mSpellsWidgetMap.find(toUpdate)->second;
            int price = mSpellsPriceMap.find(spell)->second;
            if (price>mWindowManager.getInventoryWindow()->getPlayerGold())
                toUpdate->setCaption("#A3997B" + spell->name+"   -   "+boost::lexical_cast<std::string>(price)+MWBase::Environment::get().getWorld()->getStore().gameSettings.search("sgp")->str);
            else
                toUpdate->setCaption(spell->name+"   -   "+boost::lexical_cast<std::string>(price)+MWBase::Environment::get().getWorld()->getStore().gameSettings.search("sgp")->str);
        }
    }

    void SpellsWindow::onReferenceUnavailable()
    {
        // remove both Spells and Dialogue (since you always trade with the NPC/creature that you have previously talked to)
        mWindowManager.removeGuiMode(GM_Spells);
        mWindowManager.removeGuiMode(GM_Dialogue);
    }

    void SpellsWindow::updateScroller()
    {
        mSpellsScrollerWidget->setScrollRange(std::max(mCurrentY - mSpellsClientWidget->getHeight(), 0));
        mSpellsScrollerWidget->setScrollPage(std::max(mSpellsClientWidget->getHeight() - sLineHeight, 0));
        if (mCurrentY != 0)
            mSpellsScrollerWidget->setTrackSize( (mSpellsBoxWidget->getHeight() / float(mCurrentY)) * mSpellsScrollerWidget->getLineSize() );
    }

    void SpellsWindow::onScrollChangePosition(MyGUI::ScrollBar* scroller, size_t pos)
    {
        int diff = mLastPos - pos;
        // Adjust position of all widget according to difference
        if (diff == 0)
            return;
        mLastPos = pos;

        std::vector<MyGUI::WidgetPtr>::const_iterator end = mSpellsWidgets.end();
        for (std::vector<MyGUI::WidgetPtr>::const_iterator it = mSpellsWidgets.begin(); it != end; ++it)
        {
            (*it)->setCoord((*it)->getCoord() + MyGUI::IntPoint(0, diff));
        }
    }

    void SpellsWindow::onMouseWheel(MyGUI::Widget* _sender, int _rel)
    {
        if (mSpellsScrollerWidget->getScrollPosition() - _rel*0.3 < 0)
            mSpellsScrollerWidget->setScrollPosition(0);
        else if (mSpellsScrollerWidget->getScrollPosition() - _rel*0.3 > mSpellsScrollerWidget->getScrollRange()-1)
            mSpellsScrollerWidget->setScrollPosition(mSpellsScrollerWidget->getScrollRange()-1);
        else
            mSpellsScrollerWidget->setScrollPosition(mSpellsScrollerWidget->getScrollPosition() - _rel*0.3);

        onScrollChangePosition(mSpellsScrollerWidget, mSpellsScrollerWidget->getScrollPosition());
    }
}

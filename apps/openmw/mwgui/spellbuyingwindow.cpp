#include "spellbuyingwindow.hpp"

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
    const int SpellBuyingWindow::sLineHeight = 18;

    SpellBuyingWindow::SpellBuyingWindow(MWBase::WindowManager& parWindowManager) :
        WindowBase("openmw_spell_buying_window.layout", parWindowManager)
        , ContainerBase(NULL) // no drag&drop
        , mCurrentY(0)
        , mLastPos(0)
    {
        setCoord(0, 0, 450, 300);

        getWidget(mCancelButton, "CancelButton");
        getWidget(mPlayerGold, "PlayerGold");
        getWidget(mSelect, "Select");
        getWidget(mSpells, "Spells");
        getWidget(mSpellsBoxWidget, "SpellsBox");
        getWidget(mSpellsClientWidget, "SpellsClient");
        getWidget(mSpellsScrollerWidget, "SpellsScroller");

        mSpellsClientWidget->eventMouseWheel += MyGUI::newDelegate(this, &SpellBuyingWindow::onMouseWheel);

        mSpellsScrollerWidget->eventScrollChangePosition += MyGUI::newDelegate(this, &SpellBuyingWindow::onScrollChangePosition);

        mCancelButton->eventMouseButtonClick += MyGUI::newDelegate(this, &SpellBuyingWindow::onCancelButtonClicked);

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
    }

    void SpellBuyingWindow::addSpell(const std::string& spellId)
    {
        const ESM::Spell* spell = MWBase::Environment::get().getWorld()->getStore().spells.find(spellId);
        int price = spell->data.cost*MWBase::Environment::get().getWorld()->getStore().gameSettings.search("fSpellValueMult")->f;
        MyGUI::Button* toAdd = mSpellsClientWidget->createWidget<MyGUI::Button>((price>mWindowManager.getInventoryWindow()->getPlayerGold()) ? "SandTextGreyedOut" : "SpellText", 0, mCurrentY, 200, sLineHeight, MyGUI::Align::Default);
        mCurrentY += sLineHeight;
        /// \todo price adjustment depending on merchantile skill
        toAdd->setUserData(price);
        toAdd->setCaption(spell->name+"   -   "+boost::lexical_cast<std::string>(price)+MWBase::Environment::get().getWorld()->getStore().gameSettings.search("sgp")->str);
        toAdd->setSize(toAdd->getTextSize().width,sLineHeight);
        toAdd->eventMouseWheel += MyGUI::newDelegate(this, &SpellBuyingWindow::onMouseWheel);
        toAdd->setUserString("ToolTipType", "Spell");
        toAdd->setUserString("Spell", spellId);
        toAdd->eventMouseButtonClick += MyGUI::newDelegate(this, &SpellBuyingWindow::onSpellButtonClick);
        mSpellsWidgetMap.insert(std::make_pair (toAdd, spellId));
    }

    void SpellBuyingWindow::clearSpells()
    {
        mSpellsScrollerWidget->setScrollPosition(0);
        onScrollChangePosition(mSpellsScrollerWidget, mSpellsScrollerWidget->getScrollPosition());
        mCurrentY = 0;
        while (mSpellsClientWidget->getChildCount())
            MyGUI::Gui::getInstance().destroyWidget(mSpellsClientWidget->getChildAt(0));
        mSpellsWidgetMap.clear();
    }

    void SpellBuyingWindow::startSpellBuying(const MWWorld::Ptr& actor)
    {
        center();
        mActor = actor;
        clearSpells();

        MWWorld::Ptr player = MWBase::Environment::get().getWorld()->getPlayer().getPlayer();

        MWMechanics::Spells& playerSpells = MWWorld::Class::get (player).getCreatureStats (player).getSpells();
        MWMechanics::Spells& merchantSpells = MWWorld::Class::get (actor).getCreatureStats (actor).getSpells();
         
        for (MWMechanics::Spells::TIterator iter = merchantSpells.begin(); iter!=merchantSpells.end(); ++iter)
        {
            const ESM::Spell* spell = MWBase::Environment::get().getWorld()->getStore().spells.find (*iter);
            
            if (spell->data.type!=ESM::Spell::ST_Spell)
                continue; // don't try to sell diseases, curses or powers
            
            if (std::find (playerSpells.begin(), playerSpells.end(), *iter)!=playerSpells.end())
                continue; // we have that spell already
            
            addSpell (*iter);
        }

        updateLabels();
        updateScroller();
    }

    void SpellBuyingWindow::onSpellButtonClick(MyGUI::Widget* _sender)
    {
        int price = *_sender->getUserData<int>();

        if (mWindowManager.getInventoryWindow()->getPlayerGold()>=price)
        {
            MWWorld::Ptr player = MWBase::Environment::get().getWorld()->getPlayer().getPlayer();
            MWMechanics::CreatureStats& stats = MWWorld::Class::get(player).getCreatureStats(player);
            MWMechanics::Spells& spells = stats.getSpells();
            spells.add (mSpellsWidgetMap.find(_sender)->second);
            mWindowManager.getTradeWindow()->addOrRemoveGold(-price);
            mSpellsScrollerWidget->setScrollPosition(0);
            onScrollChangePosition(mSpellsScrollerWidget, mSpellsScrollerWidget->getScrollPosition());
            updateScroller();
            startSpellBuying(mActor);
        }
    }

    void SpellBuyingWindow::onCancelButtonClicked(MyGUI::Widget* _sender)
    {
        mWindowManager.removeGuiMode(GM_SpellBuying);
    }

    void SpellBuyingWindow::updateLabels()
    {
        mPlayerGold->setCaption(MWBase::Environment::get().getWorld()->getStore().gameSettings.search("sGold")->str
            + ": " + boost::lexical_cast<std::string>(mWindowManager.getInventoryWindow()->getPlayerGold()));
        mPlayerGold->setCoord(8,
                              mPlayerGold->getTop(),
                              mPlayerGold->getTextSize().width,
                              mPlayerGold->getHeight());
    }

    void SpellBuyingWindow::onReferenceUnavailable()
    {
        // remove both Spells and Dialogue (since you always trade with the NPC/creature that you have previously talked to)
        mWindowManager.removeGuiMode(GM_SpellBuying);
        mWindowManager.removeGuiMode(GM_Dialogue);
    }

    void SpellBuyingWindow::updateScroller()
    {
        mSpellsScrollerWidget->setScrollRange(std::max(mCurrentY - mSpellsClientWidget->getHeight(), 0));
        mSpellsScrollerWidget->setScrollPage(std::max(mSpellsClientWidget->getHeight() - sLineHeight, 0));
        if (mCurrentY != 0)
            mSpellsScrollerWidget->setTrackSize( (mSpellsBoxWidget->getHeight() / float(mCurrentY)) * mSpellsScrollerWidget->getLineSize() );
    }

    void SpellBuyingWindow::onScrollChangePosition(MyGUI::ScrollBar* scroller, size_t pos)
    {
        int diff = mLastPos - pos;
        // Adjust position of all widget according to difference
        if (diff == 0)
            return;
        mLastPos = pos;

        for (unsigned int i=0;i<mSpellsClientWidget->getChildCount();i++)
        {
            MyGUI::Widget* toMove = mSpellsClientWidget->getChildAt(i);
            toMove->setCoord(toMove->getCoord() + MyGUI::IntPoint(0, diff));
        }
    }

    void SpellBuyingWindow::onMouseWheel(MyGUI::Widget* _sender, int _rel)
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


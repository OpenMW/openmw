#include "spellbuyingwindow.hpp"

#include <algorithm>

#include <boost/lexical_cast.hpp>

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
    const int SpellBuyingWindow::sLineHeight = 18;

    SpellBuyingWindow::SpellBuyingWindow(MWBase::WindowManager& parWindowManager) :
        WindowBase("openmw_spell_buying_window.layout", parWindowManager)
        , mCurrentY(0)
        , mLastPos(0)
    {
        setCoord(0, 0, 450, 300);

        getWidget(mCancelButton, "CancelButton");
        getWidget(mPlayerGold, "PlayerGold");
        getWidget(mSpellsView, "SpellsView");

        mCancelButton->eventMouseButtonClick += MyGUI::newDelegate(this, &SpellBuyingWindow::onCancelButtonClicked);
    }

    void SpellBuyingWindow::addSpell(const std::string& spellId)
    {
        const MWWorld::ESMStore &store =
            MWBase::Environment::get().getWorld()->getStore();

        const ESM::Spell* spell = MWBase::Environment::get().getWorld()->getStore().get<ESM::Spell>().find(spellId);
        int price = spell->mData.mCost*store.get<ESM::GameSetting>().find("fSpellValueMult")->getFloat();
        price = MWBase::Environment::get().getMechanicsManager()->getBarterOffer(mPtr,price,true);

        MyGUI::Button* toAdd =
            mSpellsView->createWidget<MyGUI::Button>(
                (price>mWindowManager.getInventoryWindow()->getPlayerGold()) ? "SandTextGreyedOut" : "SandTextButton",
                0,
                mCurrentY,
                200,
                sLineHeight,
                MyGUI::Align::Default
            );

        mCurrentY += sLineHeight;

        toAdd->setUserData(price);
        toAdd->setCaptionWithReplacing(spell->mName+"   -   "+boost::lexical_cast<std::string>(price)+"#{sgp}");
        toAdd->setSize(toAdd->getTextSize().width,sLineHeight);
        toAdd->eventMouseWheel += MyGUI::newDelegate(this, &SpellBuyingWindow::onMouseWheel);
        toAdd->setUserString("ToolTipType", "Spell");
        toAdd->setUserString("Spell", spellId);
        toAdd->eventMouseButtonClick += MyGUI::newDelegate(this, &SpellBuyingWindow::onSpellButtonClick);
        mSpellsWidgetMap.insert(std::make_pair (toAdd, spellId));
    }

    void SpellBuyingWindow::clearSpells()
    {
        mSpellsView->setViewOffset(MyGUI::IntPoint(0,0));
        mCurrentY = 0;
        while (mSpellsView->getChildCount())
            MyGUI::Gui::getInstance().destroyWidget(mSpellsView->getChildAt(0));
        mSpellsWidgetMap.clear();
    }

    void SpellBuyingWindow::startSpellBuying(const MWWorld::Ptr& actor)
    {
        center();
        mPtr = actor;
        clearSpells();

        MWMechanics::Spells& merchantSpells = MWWorld::Class::get (actor).getCreatureStats (actor).getSpells();

        for (MWMechanics::Spells::TIterator iter = merchantSpells.begin(); iter!=merchantSpells.end(); ++iter)
        {
            const ESM::Spell* spell =
                MWBase::Environment::get().getWorld()->getStore().get<ESM::Spell>().find (iter->first);

            if (spell->mData.mType!=ESM::Spell::ST_Spell)
                continue; // don't try to sell diseases, curses or powers

            if (playerHasSpell(iter->first))
                continue;

            addSpell (iter->first);
        }

        updateLabels();

        mSpellsView->setCanvasSize (MyGUI::IntSize(mSpellsView->getWidth(), std::max(mSpellsView->getHeight(), mCurrentY)));
    }

    bool SpellBuyingWindow::playerHasSpell(const std::string &id)
    {
        MWWorld::Ptr player = MWBase::Environment::get().getWorld()->getPlayer().getPlayer();
        MWMechanics::Spells& playerSpells = MWWorld::Class::get (player).getCreatureStats (player).getSpells();
        for (MWMechanics::Spells::TIterator it = playerSpells.begin(); it != playerSpells.end(); ++it)
        {
            if (Misc::StringUtils::ciEqual(id, it->first))
                return true;
        }
        return false;
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
            startSpellBuying(mPtr);

            MWBase::Environment::get().getSoundManager()->playSound ("Item Gold Up", 1.0, 1.0);
        }
    }

    void SpellBuyingWindow::onCancelButtonClicked(MyGUI::Widget* _sender)
    {
        mWindowManager.removeGuiMode(GM_SpellBuying);
    }

    void SpellBuyingWindow::updateLabels()
    {
        mPlayerGold->setCaptionWithReplacing("#{sGold}: " + boost::lexical_cast<std::string>(mWindowManager.getInventoryWindow()->getPlayerGold()));
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

    void SpellBuyingWindow::onMouseWheel(MyGUI::Widget* _sender, int _rel)
    {
        if (mSpellsView->getViewOffset().top + _rel*0.3 > 0)
            mSpellsView->setViewOffset(MyGUI::IntPoint(0, 0));
        else
            mSpellsView->setViewOffset(MyGUI::IntPoint(0, mSpellsView->getViewOffset().top + _rel*0.3));
    }
}


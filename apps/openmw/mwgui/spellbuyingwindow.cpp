#include "spellbuyingwindow.hpp"

#include <MyGUI_Gui.h>
#include <MyGUI_Button.h>
#include <MyGUI_ScrollView.h>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwbase/soundmanager.hpp"
#include "../mwbase/windowmanager.hpp"
#include "../mwbase/mechanicsmanager.hpp"

#include "../mwworld/class.hpp"
#include "../mwworld/containerstore.hpp"
#include "../mwworld/esmstore.hpp"

#include "../mwmechanics/creaturestats.hpp"
#include "../mwmechanics/actorutil.hpp"

namespace MWGui
{
    const int SpellBuyingWindow::sLineHeight = 18;

    SpellBuyingWindow::SpellBuyingWindow() :
        WindowBase("openmw_spell_buying_window.layout")
        , mLastPos(0)
        , mCurrentY(0)
    {
        setCoord(0, 0, 450, 300);

        getWidget(mCancelButton, "CancelButton");
        getWidget(mPlayerGold, "PlayerGold");
        getWidget(mSpellsView, "SpellsView");

        mCancelButton->eventMouseButtonClick += MyGUI::newDelegate(this, &SpellBuyingWindow::onCancelButtonClicked);
    }

    void SpellBuyingWindow::exit()
    {
        MWBase::Environment::get().getWindowManager()->removeGuiMode(GM_SpellBuying);
    }

    void SpellBuyingWindow::addSpell(const std::string& spellId)
    {
        const MWWorld::ESMStore &store =
            MWBase::Environment::get().getWorld()->getStore();

        const ESM::Spell* spell = MWBase::Environment::get().getWorld()->getStore().get<ESM::Spell>().find(spellId);
        int price = static_cast<int>(spell->mData.mCost*store.get<ESM::GameSetting>().find("fSpellValueMult")->getFloat());
        price = MWBase::Environment::get().getMechanicsManager()->getBarterOffer(mPtr,price,true);

        MWWorld::Ptr player = MWMechanics::getPlayer();
        int playerGold = player.getClass().getContainerStore(player).count(MWWorld::ContainerStore::sGoldId);

        // TODO: refactor to use MyGUI::ListBox

        MyGUI::Button* toAdd =
            mSpellsView->createWidget<MyGUI::Button>(
                price <= playerGold ? "SandTextButton" : "SandTextButtonDisabled", // can't use setEnabled since that removes tooltip
                0,
                mCurrentY,
                200,
                sLineHeight,
                MyGUI::Align::Default
            );

        mCurrentY += sLineHeight;

        toAdd->setUserData(price);
        toAdd->setCaptionWithReplacing(spell->mName+"   -   "+MyGUI::utility::toString(price)+"#{sgp}");
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

        MWMechanics::Spells& merchantSpells = actor.getClass().getCreatureStats (actor).getSpells();

        for (MWMechanics::Spells::TIterator iter = merchantSpells.begin(); iter!=merchantSpells.end(); ++iter)
        {
            const ESM::Spell* spell =
                MWBase::Environment::get().getWorld()->getStore().get<ESM::Spell>().find (iter->first);

            if (spell->mData.mType!=ESM::Spell::ST_Spell)
                continue; // don't try to sell diseases, curses or powers

            if (actor.getClass().isNpc())
            {
                const ESM::Race* race =
                        MWBase::Environment::get().getWorld()->getStore().get<ESM::Race>().find(
                        actor.get<ESM::NPC>()->mBase->mRace);
                if (race->mPowers.exists(spell->mId))
                    continue;
            }

            if (playerHasSpell(iter->first))
                continue;

            addSpell (iter->first);
        }

        updateLabels();

        // Canvas size must be expressed with VScroll disabled, otherwise MyGUI would expand the scroll area when the scrollbar is hidden
        mSpellsView->setVisibleVScroll(false);
        mSpellsView->setCanvasSize (MyGUI::IntSize(mSpellsView->getWidth(), std::max(mSpellsView->getHeight(), mCurrentY)));
        mSpellsView->setVisibleVScroll(true);
    }

    bool SpellBuyingWindow::playerHasSpell(const std::string &id)
    {
        MWWorld::Ptr player = MWMechanics::getPlayer();
        return player.getClass().getCreatureStats(player).getSpells().hasSpell(id);
    }

    void SpellBuyingWindow::onSpellButtonClick(MyGUI::Widget* _sender)
    {
        int price = *_sender->getUserData<int>();

        MWWorld::Ptr player = MWMechanics::getPlayer();
        if (price > player.getClass().getContainerStore(player).count(MWWorld::ContainerStore::sGoldId))
            return;

        MWMechanics::CreatureStats& stats = player.getClass().getCreatureStats(player);
        MWMechanics::Spells& spells = stats.getSpells();
        spells.add (mSpellsWidgetMap.find(_sender)->second);
        player.getClass().getContainerStore(player).remove(MWWorld::ContainerStore::sGoldId, price, player);

        // add gold to NPC trading gold pool
        MWMechanics::CreatureStats& npcStats = mPtr.getClass().getCreatureStats(mPtr);
        npcStats.setGoldPool(npcStats.getGoldPool() + price);

        startSpellBuying(mPtr);

        MWBase::Environment::get().getSoundManager()->playSound ("Item Gold Up", 1.0, 1.0);
    }

    void SpellBuyingWindow::onCancelButtonClicked(MyGUI::Widget* _sender)
    {
        exit();
    }

    void SpellBuyingWindow::updateLabels()
    {
        MWWorld::Ptr player = MWMechanics::getPlayer();
        int playerGold = player.getClass().getContainerStore(player).count(MWWorld::ContainerStore::sGoldId);

        mPlayerGold->setCaptionWithReplacing("#{sGold}: " + MyGUI::utility::toString(playerGold));
        mPlayerGold->setCoord(8,
                              mPlayerGold->getTop(),
                              mPlayerGold->getTextSize().width,
                              mPlayerGold->getHeight());
    }

    void SpellBuyingWindow::onReferenceUnavailable()
    {
        // remove both Spells and Dialogue (since you always trade with the NPC/creature that you have previously talked to)
        MWBase::Environment::get().getWindowManager()->removeGuiMode(GM_SpellBuying);
        MWBase::Environment::get().getWindowManager()->removeGuiMode(GM_Dialogue);
    }

    void SpellBuyingWindow::onMouseWheel(MyGUI::Widget* _sender, int _rel)
    {
        if (mSpellsView->getViewOffset().top + _rel*0.3 > 0)
            mSpellsView->setViewOffset(MyGUI::IntPoint(0, 0));
        else
            mSpellsView->setViewOffset(MyGUI::IntPoint(0, static_cast<int>(mSpellsView->getViewOffset().top + _rel*0.3f)));
    }
}


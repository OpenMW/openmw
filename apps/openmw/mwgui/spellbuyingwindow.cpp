#include "spellbuyingwindow.hpp"

#include <MyGUI_Button.h>
#include <MyGUI_Gui.h>
#include <MyGUI_ScrollView.h>

#include <components/esm3/loadgmst.hpp>
#include <components/esm3/loadrace.hpp>
#include <components/settings/values.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/inputmanager.hpp"
#include "../mwbase/mechanicsmanager.hpp"
#include "../mwbase/windowmanager.hpp"

#include "../mwworld/class.hpp"
#include "../mwworld/containerstore.hpp"
#include "../mwworld/esmstore.hpp"

#include "../mwmechanics/actorutil.hpp"
#include "../mwmechanics/creaturestats.hpp"
#include "../mwmechanics/spells.hpp"
#include "../mwmechanics/spellutil.hpp"

namespace MWGui
{
    SpellBuyingWindow::SpellBuyingWindow()
        : WindowBase("openmw_spell_buying_window.layout")
        , mCurrentY(0)
        , mControllerFocus(0)
    {
        getWidget(mCancelButton, "CancelButton");
        getWidget(mPlayerGold, "PlayerGold");
        getWidget(mSpellsView, "SpellsView");

        mCancelButton->eventMouseButtonClick += MyGUI::newDelegate(this, &SpellBuyingWindow::onCancelButtonClicked);

        if (Settings::gui().mControllerMenus)
        {
            mDisableGamepadCursor = true;
            mControllerButtons.mA = "#{Interface:Buy}";
            mControllerButtons.mB = "#{Interface:Cancel}";
            mControllerButtons.mR3 = "#{Interface:Info}";
        }
    }

    bool SpellBuyingWindow::sortSpells(const ESM::Spell* left, const ESM::Spell* right)
    {
        return Misc::StringUtils::ciLess(left->mName, right->mName);
    }

    void SpellBuyingWindow::addSpell(const ESM::Spell& spell)
    {
        const MWWorld::ESMStore& store = *MWBase::Environment::get().getESMStore();

        int price = std::max(1,
            static_cast<int>(MWMechanics::calcSpellCost(spell)
                * store.get<ESM::GameSetting>().find("fSpellValueMult")->mValue.getFloat()));
        price = MWBase::Environment::get().getMechanicsManager()->getBarterOffer(mPtr, price, true);

        MWWorld::Ptr player = MWMechanics::getPlayer();
        int playerGold = player.getClass().getContainerStore(player).count(MWWorld::ContainerStore::sGoldId);

        // TODO: refactor to use MyGUI::ListBox

        const int lineHeight = Settings::gui().mFontSize + 2;

        MyGUI::Button* toAdd = mSpellsView->createWidget<MyGUI::Button>(price <= playerGold
                ? "SandTextButton"
                : "SandTextButtonDisabled", // can't use setEnabled since that removes tooltip
            0, mCurrentY, 200, lineHeight, MyGUI::Align::Default);

        mCurrentY += lineHeight;

        toAdd->setUserData(price);
        toAdd->setCaptionWithReplacing(spell.mName + "  - " + MyGUI::utility::toString(price) + "#{sgp}");
        toAdd->setSize(mSpellsView->getWidth(), lineHeight);
        toAdd->eventMouseWheel += MyGUI::newDelegate(this, &SpellBuyingWindow::onMouseWheel);
        toAdd->setUserString("ToolTipType", "Spell");
        toAdd->setUserString("Spell", spell.mId.serialize());
        toAdd->setUserString("SpellCost", "true");
        toAdd->eventMouseButtonClick += MyGUI::newDelegate(this, &SpellBuyingWindow::onSpellButtonClick);
        mSpellsWidgetMap.insert(std::make_pair(toAdd, spell.mId));
        if (price <= playerGold)
            mSpellButtons.emplace_back(std::make_pair(toAdd, mSpellsWidgetMap.size()));
    }

    void SpellBuyingWindow::clearSpells()
    {
        mSpellsView->setViewOffset(MyGUI::IntPoint(0, 0));
        mCurrentY = 0;
        while (mSpellsView->getChildCount())
            MyGUI::Gui::getInstance().destroyWidget(mSpellsView->getChildAt(0));
        mSpellsWidgetMap.clear();
        mSpellButtons.clear();
    }

    void SpellBuyingWindow::setPtr(const MWWorld::Ptr& actor)
    {
        setPtr(actor, 0);
    }

    void SpellBuyingWindow::setPtr(const MWWorld::Ptr& actor, int startOffset)
    {
        if (actor.isEmpty() || !actor.getClass().isActor())
            throw std::runtime_error("Invalid argument in SpellBuyingWindow::setPtr");

        center();
        mPtr = actor;
        clearSpells();

        MWMechanics::Spells& merchantSpells = actor.getClass().getCreatureStats(actor).getSpells();

        std::vector<const ESM::Spell*> spellsToSort;

        for (const ESM::Spell* spell : merchantSpells)
        {
            if (spell->mData.mType != ESM::Spell::ST_Spell)
                continue; // don't try to sell diseases, curses or powers

            if (actor.getClass().isNpc())
            {
                const ESM::Race* race = MWBase::Environment::get().getESMStore()->get<ESM::Race>().find(
                    actor.get<ESM::NPC>()->mBase->mRace);
                if (race->mPowers.exists(spell->mId))
                    continue;
            }

            if (playerHasSpell(spell->mId))
                continue;

            spellsToSort.push_back(spell);
        }

        std::stable_sort(spellsToSort.begin(), spellsToSort.end(), sortSpells);

        for (const ESM::Spell* spell : spellsToSort)
        {
            addSpell(*spell);
        }

        spellsToSort.clear();

        updateLabels();

        if (Settings::gui().mControllerMenus)
        {
            mControllerFocus = 0;
            if (mSpellButtons.size() > 0)
            {
                mSpellButtons[0].first->setStateSelected(true);

                MWBase::WindowManager* winMgr = MWBase::Environment::get().getWindowManager();
                winMgr->setControllerTooltipVisible(Settings::gui().mControllerTooltips);
                if (winMgr->getControllerTooltipVisible())
                    MWBase::Environment::get().getInputManager()->warpMouseToWidget(mSpellButtons[0].first);
            }
        }

        // Canvas size must be expressed with VScroll disabled, otherwise MyGUI would expand the scroll area when the
        // scrollbar is hidden
        mSpellsView->setVisibleVScroll(false);
        mSpellsView->setCanvasSize(
            MyGUI::IntSize(mSpellsView->getWidth(), std::max(mSpellsView->getHeight(), mCurrentY)));
        mSpellsView->setVisibleVScroll(true);
        mSpellsView->setViewOffset(MyGUI::IntPoint(0, startOffset));
    }

    bool SpellBuyingWindow::playerHasSpell(const ESM::RefId& id)
    {
        MWWorld::Ptr player = MWMechanics::getPlayer();
        return player.getClass().getCreatureStats(player).getSpells().hasSpell(id);
    }

    void SpellBuyingWindow::onSpellButtonClick(MyGUI::Widget* sender)
    {
        int price = *sender->getUserData<int>();

        MWWorld::Ptr player = MWMechanics::getPlayer();
        if (price > player.getClass().getContainerStore(player).count(MWWorld::ContainerStore::sGoldId))
            return;

        MWMechanics::CreatureStats& stats = player.getClass().getCreatureStats(player);
        MWMechanics::Spells& spells = stats.getSpells();
        auto spell = mSpellsWidgetMap.find(sender);
        assert(spell != mSpellsWidgetMap.end());

        spells.add(spell->second);
        player.getClass().getContainerStore(player).remove(MWWorld::ContainerStore::sGoldId, price);

        // add gold to NPC trading gold pool
        MWMechanics::CreatureStats& npcStats = mPtr.getClass().getCreatureStats(mPtr);
        npcStats.setGoldPool(npcStats.getGoldPool() + price);

        setPtr(mPtr, mSpellsView->getViewOffset().top);

        MWBase::Environment::get().getWindowManager()->playSound(ESM::RefId::stringRefId("Item Gold Up"));
    }

    void SpellBuyingWindow::onCancelButtonClicked(MyGUI::Widget* /*sender*/)
    {
        MWBase::Environment::get().getWindowManager()->removeGuiMode(MWGui::GM_SpellBuying);
    }

    void SpellBuyingWindow::updateLabels()
    {
        MWWorld::Ptr player = MWMechanics::getPlayer();
        int playerGold = player.getClass().getContainerStore(player).count(MWWorld::ContainerStore::sGoldId);

        mPlayerGold->setCaptionWithReplacing("#{sGold}: " + MyGUI::utility::toString(playerGold));
        mPlayerGold->setCoord(8, mPlayerGold->getTop(), mPlayerGold->getTextSize().width, mPlayerGold->getHeight());
    }

    void SpellBuyingWindow::onReferenceUnavailable()
    {
        // remove both Spells and Dialogue (since you always trade with the NPC/creature that you have previously talked
        // to)
        MWBase::Environment::get().getWindowManager()->removeGuiMode(GM_SpellBuying);
        MWBase::Environment::get().getWindowManager()->exitCurrentGuiMode();
    }

    void SpellBuyingWindow::onMouseWheel(MyGUI::Widget* /*sender*/, int rel)
    {
        if (mSpellsView->getViewOffset().top + rel * 0.3 > 0)
            mSpellsView->setViewOffset(MyGUI::IntPoint(0, 0));
        else
            mSpellsView->setViewOffset(
                MyGUI::IntPoint(0, static_cast<int>(mSpellsView->getViewOffset().top + rel * 0.3f)));
    }

    bool SpellBuyingWindow::onControllerButtonEvent(const SDL_ControllerButtonEvent& arg)
    {
        MWBase::WindowManager* winMgr = MWBase::Environment::get().getWindowManager();

        if (arg.button == SDL_CONTROLLER_BUTTON_A)
        {
            if (mControllerFocus < mSpellButtons.size())
                onSpellButtonClick(mSpellButtons[mControllerFocus].first);
        }
        else if (arg.button == SDL_CONTROLLER_BUTTON_B)
        {
            onCancelButtonClicked(mCancelButton);
        }
        else if (arg.button == SDL_CONTROLLER_BUTTON_RIGHTSTICK)
        {
            // Toggle info tooltip
            winMgr->setControllerTooltipEnabled(!winMgr->getControllerTooltipEnabled());
        }
        else if (arg.button == SDL_CONTROLLER_BUTTON_DPAD_UP)
        {
            winMgr->restoreControllerTooltips();

            if (mSpellButtons.size() <= 1)
                return true;

            mSpellButtons[mControllerFocus].first->setStateSelected(false);
            mControllerFocus = wrap(mControllerFocus, mSpellButtons.size(), -1);
            mSpellButtons[mControllerFocus].first->setStateSelected(true);
        }
        else if (arg.button == SDL_CONTROLLER_BUTTON_DPAD_DOWN)
        {
            winMgr->restoreControllerTooltips();

            if (mSpellButtons.size() <= 1)
                return true;

            mSpellButtons[mControllerFocus].first->setStateSelected(false);
            mControllerFocus = wrap(mControllerFocus, mSpellButtons.size(), 1);
            mSpellButtons[mControllerFocus].first->setStateSelected(true);
        }
        else
            return true;

        if (mControllerFocus < mSpellButtons.size())
        {
            // Scroll the list to keep the active item in view
            size_t line = mSpellButtons[mControllerFocus].second;
            if (line <= 5)
                mSpellsView->setViewOffset(MyGUI::IntPoint(0, 0));
            else
            {
                const int lineHeight = Settings::gui().mFontSize + 2;
                mSpellsView->setViewOffset(MyGUI::IntPoint(0, -lineHeight * static_cast<int>(line - 5)));
            }

            // Warp the mouse to the selected spell to show the tooltip
            if (MWBase::Environment::get().getWindowManager()->getControllerTooltipVisible())
                MWBase::Environment::get().getInputManager()->warpMouseToWidget(mSpellButtons[mControllerFocus].first);
        }

        return true;
    }
}

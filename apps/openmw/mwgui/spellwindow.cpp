#include "spellwindow.hpp"

#include <boost/lexical_cast.hpp>
#include <boost/format.hpp>

#include "../mwbase/windowmanager.hpp"
#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"

#include "../mwworld/inventorystore.hpp"
#include "../mwworld/class.hpp"

#include "../mwmechanics/spellcasting.hpp"
#include "../mwmechanics/spells.hpp"
#include "../mwmechanics/creaturestats.hpp"

#include "spellicons.hpp"
#include "inventorywindow.hpp"
#include "confirmationdialog.hpp"

namespace MWGui
{

    bool sortItems(const MWWorld::Ptr& left, const MWWorld::Ptr& right)
    {
        int cmp = left.getClass().getName(left).compare(
                    right.getClass().getName(right));
        return cmp < 0;
    }

    bool sortSpells(const std::string& left, const std::string& right)
    {
        const MWWorld::Store<ESM::Spell> &spells =
            MWBase::Environment::get().getWorld()->getStore().get<ESM::Spell>();

        const ESM::Spell* a = spells.find(left);
        const ESM::Spell* b = spells.find(right);

        int cmp = a->mName.compare(b->mName);
        return cmp < 0;
    }

    SpellWindow::SpellWindow(DragAndDrop* drag)
        : WindowPinnableBase("openmw_spell_window.layout")
        , NoDrop(drag, mMainWidget)
        , mHeight(0)
        , mWidth(0)
    {
        mSpellIcons = new SpellIcons();

        getWidget(mSpellView, "SpellView");
        getWidget(mEffectBox, "EffectsBox");

        setCoord(498, 300, 302, 300);

        mMainWidget->castType<MyGUI::Window>()->eventWindowChangeCoord += MyGUI::newDelegate(this, &SpellWindow::onWindowResize);
    }

    SpellWindow::~SpellWindow()
    {
        delete mSpellIcons;
    }

    void SpellWindow::onPinToggled()
    {
        MWBase::Environment::get().getWindowManager()->setSpellVisibility(!mPinned);
    }

    void SpellWindow::open()
    {
        updateSpells();
    }

    void SpellWindow::updateSpells()
    {
        mSpellIcons->updateWidgets(mEffectBox, false);

        const int spellHeight = 18;

        mHeight = 0;
        while (mSpellView->getChildCount())
            MyGUI::Gui::getInstance().destroyWidget(mSpellView->getChildAt(0));

        // retrieve all player spells, divide them into Powers and Spells and sort them
        std::vector<std::string> spellList;
        MWWorld::Ptr player = MWBase::Environment::get().getWorld()->getPlayerPtr();
        MWWorld::InventoryStore& store = player.getClass().getInventoryStore(player);
        MWMechanics::CreatureStats& stats = player.getClass().getCreatureStats(player);
        MWMechanics::Spells& spells = stats.getSpells();

        for (MWMechanics::Spells::TIterator it = spells.begin(); it != spells.end(); ++it)
            spellList.push_back (it->first);

        const MWWorld::ESMStore &esmStore =
            MWBase::Environment::get().getWorld()->getStore();

        std::vector<std::string> powers;
        std::vector<std::string>::iterator it = spellList.begin();
        while (it != spellList.end())
        {
            const ESM::Spell* spell = esmStore.get<ESM::Spell>().find(*it);

            if (spell->mData.mType == ESM::Spell::ST_Power)
            {
                powers.push_back(*it);
                it = spellList.erase(it);
            }
            else if (spell->mData.mType == ESM::Spell::ST_Ability
                || spell->mData.mType == ESM::Spell::ST_Blight
                || spell->mData.mType == ESM::Spell::ST_Curse
                || spell->mData.mType == ESM::Spell::ST_Disease)
            {
                it = spellList.erase(it);
            }
            else
                ++it;
        }
        std::sort(powers.begin(), powers.end(), sortSpells);
        std::sort(spellList.begin(), spellList.end(), sortSpells);

        // retrieve player's enchanted items
        std::vector<MWWorld::Ptr> items;
        for (MWWorld::ContainerStoreIterator it(store.begin()); it != store.end(); ++it)
        {
            std::string enchantId = it->getClass().getEnchantment(*it);
            if (enchantId != "")
            {
                // only add items with "Cast once" or "Cast on use"
                const ESM::Enchantment* enchant =
                    esmStore.get<ESM::Enchantment>().find(enchantId);

                int type = enchant->mData.mType;
                if (type != ESM::Enchantment::CastOnce
                    && type != ESM::Enchantment::WhenUsed)
                    continue;

                items.push_back(*it);
            }
        }
        std::sort(items.begin(), items.end(), sortItems);


        int height = estimateHeight(items.size() + powers.size() + spellList.size());
        bool scrollVisible = height > mSpellView->getHeight();
        mWidth = mSpellView->getWidth() - (scrollVisible ? 18 : 0);

        // powers
        addGroup("#{sPowers}", "");

        for (std::vector<std::string>::const_iterator it = powers.begin(); it != powers.end(); ++it)
        {
            const ESM::Spell* spell = esmStore.get<ESM::Spell>().find(*it);
            MyGUI::Button* t = mSpellView->createWidget<MyGUI::Button>("SpellText",
                MyGUI::IntCoord(4, mHeight, mWidth-8, spellHeight), MyGUI::Align::Left | MyGUI::Align::Top);
            t->setCaption(spell->mName);
            t->setTextAlign(MyGUI::Align::Left);
            t->setUserString("ToolTipType", "Spell");
            t->setUserString("Spell", *it);
            t->eventMouseWheel += MyGUI::newDelegate(this, &SpellWindow::onMouseWheel);
            t->eventMouseButtonClick += MyGUI::newDelegate(this, &SpellWindow::onSpellSelected);

            if (*it == MWBase::Environment::get().getWindowManager()->getSelectedSpell())
                t->setStateSelected(true);

            mHeight += spellHeight;
        }

        // other spells
        addGroup("#{sSpells}", "#{sCostChance}");
        for (std::vector<std::string>::const_iterator it = spellList.begin(); it != spellList.end(); ++it)
        {
            const ESM::Spell* spell = esmStore.get<ESM::Spell>().find(*it);
            MyGUI::Button* t = mSpellView->createWidget<MyGUI::Button>("SpellText",
                MyGUI::IntCoord(4, mHeight, mWidth-8, spellHeight), MyGUI::Align::Left | MyGUI::Align::Top);
            t->setCaption(spell->mName);
            t->setTextAlign(MyGUI::Align::Left);
            t->setUserString("ToolTipType", "Spell");
            t->setUserString("Spell", *it);
            t->eventMouseWheel += MyGUI::newDelegate(this, &SpellWindow::onMouseWheel);
            t->eventMouseButtonClick += MyGUI::newDelegate(this, &SpellWindow::onSpellSelected);
            t->setStateSelected(*it == MWBase::Environment::get().getWindowManager()->getSelectedSpell());

            // cost / success chance
            MyGUI::Button* costChance = mSpellView->createWidget<MyGUI::Button>("SpellText",
                MyGUI::IntCoord(4, mHeight, mWidth-8, spellHeight), MyGUI::Align::Left | MyGUI::Align::Top);
            std::string cost = boost::lexical_cast<std::string>(spell->mData.mCost);
            std::string chance = boost::lexical_cast<std::string>(int(MWMechanics::getSpellSuccessChance(*it, player)));
            costChance->setCaption(cost + "/" + chance);
            costChance->setTextAlign(MyGUI::Align::Right);
            costChance->setNeedMouseFocus(false);
            costChance->setStateSelected(*it == MWBase::Environment::get().getWindowManager()->getSelectedSpell());

            t->setSize(mWidth-12-costChance->getTextSize().width, t->getHeight());

            mHeight += spellHeight;
        }


        // enchanted items
        addGroup("#{sMagicItem}", "#{sCostCharge}");

        for (std::vector<MWWorld::Ptr>::const_iterator it = items.begin(); it != items.end(); ++it)
        {
            MWWorld::Ptr item = *it;

            const ESM::Enchantment* enchant =
                esmStore.get<ESM::Enchantment>().find(item.getClass().getEnchantment(item));

            // check if the item is currently equipped (will display in a different color)
            bool equipped = false;
            for (int i=0; i < MWWorld::InventoryStore::Slots; ++i)
            {
                if (store.getSlot(i) != store.end() && *store.getSlot(i) == item)
                {
                    equipped = true;
                    break;
                }
            }

            MyGUI::Button* t = mSpellView->createWidget<MyGUI::Button>(equipped ? "SpellText" : "SpellTextUnequipped",
                MyGUI::IntCoord(4, mHeight, mWidth-8, spellHeight), MyGUI::Align::Left | MyGUI::Align::Top);
            t->setCaption(item.getClass().getName(item));
            t->setTextAlign(MyGUI::Align::Left);
            t->setUserData(item);
            t->setUserString("ToolTipType", "ItemPtr");
            t->setUserString("Equipped", equipped ? "true" : "false");
            t->eventMouseButtonClick += MyGUI::newDelegate(this, &SpellWindow::onEnchantedItemSelected);
            t->eventMouseWheel += MyGUI::newDelegate(this, &SpellWindow::onMouseWheel);
            if (store.getSelectedEnchantItem() != store.end())
                t->setStateSelected(item == *store.getSelectedEnchantItem());


            // cost / charge
            MyGUI::Button* costCharge = mSpellView->createWidget<MyGUI::Button>(equipped ? "SpellText" : "SpellTextUnequipped",
                MyGUI::IntCoord(4, mHeight, mWidth-8, spellHeight), MyGUI::Align::Left | MyGUI::Align::Top);

            float enchantCost = enchant->mData.mCost;
            int eSkill = player.getClass().getSkill(player, ESM::Skill::Enchant);
            int castCost = std::max(1.f, enchantCost - (enchantCost / 100) * (eSkill - 10));

            std::string cost = boost::lexical_cast<std::string>(castCost);
            int currentCharge = int(item.getCellRef().getEnchantmentCharge());
            if (currentCharge ==  -1)
                currentCharge = enchant->mData.mCharge;
            std::string charge = boost::lexical_cast<std::string>(currentCharge);
            if (enchant->mData.mType == ESM::Enchantment::CastOnce)
            {
                // this is Morrowind behaviour
                cost = "100";
                charge = "100";
            }

            costCharge->setCaption(cost + "/" + charge);
            costCharge->setTextAlign(MyGUI::Align::Right);
            costCharge->setNeedMouseFocus(false);
            if (store.getSelectedEnchantItem() != store.end())
                costCharge->setStateSelected(item == *store.getSelectedEnchantItem());

            t->setSize(mWidth-12-costCharge->getTextSize().width, t->getHeight());

            mHeight += spellHeight;
        }

        // Canvas size must be expressed with VScroll disabled, otherwise MyGUI would expand the scroll area when the scrollbar is hidden
        mSpellView->setVisibleVScroll(false);
        mSpellView->setCanvasSize(mSpellView->getWidth(), std::max(mSpellView->getHeight(), mHeight));
        mSpellView->setVisibleVScroll(true);
    }

    void SpellWindow::addGroup(const std::string &label, const std::string& label2)
    {
        if (mSpellView->getChildCount() > 0)
        {
            MyGUI::ImageBox* separator = mSpellView->createWidget<MyGUI::ImageBox>("MW_HLine",
                MyGUI::IntCoord(4, mHeight, mWidth-8, 18),
                MyGUI::Align::Left | MyGUI::Align::Top);
            separator->setNeedMouseFocus(false);
            mHeight += 18;
        }

        MyGUI::TextBox* groupWidget = mSpellView->createWidget<MyGUI::TextBox>("SandBrightText",
            MyGUI::IntCoord(0, mHeight, mWidth, 24),
            MyGUI::Align::Left | MyGUI::Align::Top | MyGUI::Align::HStretch);
        groupWidget->setCaptionWithReplacing(label);
        groupWidget->setTextAlign(MyGUI::Align::Left);
        groupWidget->setNeedMouseFocus(false);

        if (label2 != "")
        {
            MyGUI::TextBox* groupWidget2 = mSpellView->createWidget<MyGUI::TextBox>("SandBrightText",
                MyGUI::IntCoord(0, mHeight, mWidth-4, 24),
                MyGUI::Align::Left | MyGUI::Align::Top);
            groupWidget2->setCaptionWithReplacing(label2);
            groupWidget2->setTextAlign(MyGUI::Align::Right);
            groupWidget2->setNeedMouseFocus(false);

            groupWidget->setSize(mWidth-8-groupWidget2->getTextSize().width, groupWidget->getHeight());
        }

        mHeight += 24;
    }

    void SpellWindow::onWindowResize(MyGUI::Window* _sender)
    {
        updateSpells();
    }

    void SpellWindow::onEnchantedItemSelected(MyGUI::Widget* _sender)
    {
        MWWorld::Ptr player = MWBase::Environment::get().getWorld()->getPlayerPtr();
        MWWorld::InventoryStore& store = player.getClass().getInventoryStore(player);
        MWWorld::Ptr item = *_sender->getUserData<MWWorld::Ptr>();

        // retrieve ContainerStoreIterator to the item
        MWWorld::ContainerStoreIterator it = store.begin();
        for (; it != store.end(); ++it)
        {
            if (*it == item)
            {
                break;
            }
        }
        assert(it != store.end());

        // equip, if it can be equipped and is not already equipped
        if (_sender->getUserString("Equipped") == "false"
            && !item.getClass().getEquipmentSlots(item).first.empty())
        {
            MWBase::Environment::get().getWindowManager()->getInventoryWindow()->useItem(item);
        }

        store.setSelectedEnchantItem(it);

        updateSpells();
    }

    void SpellWindow::onSpellSelected(MyGUI::Widget* _sender)
    {
        std::string spellId = _sender->getUserString("Spell");
        MWWorld::Ptr player = MWBase::Environment::get().getWorld()->getPlayerPtr();
        MWWorld::InventoryStore& store = player.getClass().getInventoryStore(player);

        if (MyGUI::InputManager::getInstance().isShiftPressed())
        {
            // delete spell, if allowed
            const ESM::Spell* spell =
                MWBase::Environment::get().getWorld()->getStore().get<ESM::Spell>().find(spellId);

            if (spell->mData.mFlags & ESM::Spell::F_Always
                || spell->mData.mType == ESM::Spell::ST_Power)
            {
                MWBase::Environment::get().getWindowManager()->messageBox("#{sDeleteSpellError}");
            }
            else
            {
                // ask for confirmation
                mSpellToDelete = spellId;
                ConfirmationDialog* dialog = MWBase::Environment::get().getWindowManager()->getConfirmationDialog();
                std::string question = MWBase::Environment::get().getWindowManager()->getGameSettingString("sQuestionDeleteSpell", "Delete %s?");
                question = boost::str(boost::format(question) % spell->mName);
                dialog->open(question);
                dialog->eventOkClicked.clear();
                dialog->eventOkClicked += MyGUI::newDelegate(this, &SpellWindow::onDeleteSpellAccept);
                dialog->eventCancelClicked.clear();
            }
        }
        else
        {
            store.setSelectedEnchantItem(store.end());
            MWBase::Environment::get().getWindowManager()->setSelectedSpell(spellId, int(MWMechanics::getSpellSuccessChance(spellId, player)));
        }

        updateSpells();
    }

    int SpellWindow::estimateHeight(int numSpells) const
    {
        int height = 0;
        height += 24 * 3 + 18 * 2; // group headings
        height += numSpells * 18;
        return height;
    }

    void SpellWindow::onMouseWheel(MyGUI::Widget* _sender, int _rel)
    {
        if (mSpellView->getViewOffset().top + _rel*0.3 > 0)
            mSpellView->setViewOffset(MyGUI::IntPoint(0, 0));
        else
            mSpellView->setViewOffset(MyGUI::IntPoint(0, mSpellView->getViewOffset().top + _rel*0.3));
    }

    void SpellWindow::onDeleteSpellAccept()
    {
        MWWorld::Ptr player = MWBase::Environment::get().getWorld()->getPlayerPtr();
        MWMechanics::CreatureStats& stats = player.getClass().getCreatureStats(player);
        MWMechanics::Spells& spells = stats.getSpells();

        if (MWBase::Environment::get().getWindowManager()->getSelectedSpell() == mSpellToDelete)
            MWBase::Environment::get().getWindowManager()->unsetSelectedSpell();

        spells.remove(mSpellToDelete);

        updateSpells();
    }
}

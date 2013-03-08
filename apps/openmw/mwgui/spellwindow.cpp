#include "spellwindow.hpp"

#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/format.hpp>

#include "../mwworld/esmstore.hpp"

#include "../mwbase/world.hpp"
#include "../mwbase/environment.hpp"
#include "../mwbase/soundmanager.hpp"
#include "../mwbase/windowmanager.hpp"

#include "../mwworld/player.hpp"
#include "../mwworld/inventorystore.hpp"
#include "../mwworld/actionequip.hpp"

#include "../mwmechanics/spells.hpp"
#include "../mwmechanics/creaturestats.hpp"
#include "../mwmechanics/spellsuccess.hpp"

#include "spellicons.hpp"
#include "inventorywindow.hpp"
#include "confirmationdialog.hpp"

namespace
{
    bool sortSpells(const std::string& left, const std::string& right)
    {
        const MWWorld::Store<ESM::Spell> &spells =
            MWBase::Environment::get().getWorld()->getStore().get<ESM::Spell>();

        const ESM::Spell* a = spells.find(left);
        const ESM::Spell* b = spells.find(right);

        int cmp = a->mName.compare(b->mName);
        return cmp < 0;
    }

    bool sortItems(const MWWorld::Ptr& left, const MWWorld::Ptr& right)
    {
        int cmp = MWWorld::Class::get(left).getName(left).compare(
                    MWWorld::Class::get(right).getName(right));
        return cmp < 0;
    }
}

namespace MWGui
{
    SpellWindow::SpellWindow(MWBase::WindowManager& parWindowManager)
        : WindowPinnableBase("openmw_spell_window.layout", parWindowManager)
        , mHeight(0)
        , mWidth(0)
    {
        mSpellIcons = new SpellIcons();

        getWidget(mSpellView, "SpellView");
        getWidget(mEffectBox, "EffectsBox");

        setCoord(498, 300, 302, 300);

        updateSpells();

        mMainWidget->castType<MyGUI::Window>()->eventWindowChangeCoord += MyGUI::newDelegate(this, &SpellWindow::onWindowResize);
    }

    SpellWindow::~SpellWindow()
    {
        delete mSpellIcons;
    }

    void SpellWindow::onPinToggled()
    {
        mWindowManager.setSpellVisibility(!mPinned);
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
        MWWorld::Ptr player = MWBase::Environment::get().getWorld()->getPlayer().getPlayer();
        MWWorld::InventoryStore& store = MWWorld::Class::get(player).getInventoryStore(player);
        MWMechanics::CreatureStats& stats = MWWorld::Class::get(player).getCreatureStats(player);
        MWMechanics::Spells& spells = stats.getSpells();

        // the following code switches between selected enchanted item and selected spell (only one of these
        // can be active at a time)
        std::string selectedSpell = spells.getSelectedSpell();
        MWWorld::Ptr selectedItem;
        if (store.getSelectedEnchantItem() != store.end())
        {
            selectedSpell = "";
            selectedItem = *store.getSelectedEnchantItem();

            bool allowSelectedItem = true;

            // make sure that the item is still in the player inventory, otherwise it can't be selected
            bool found = false;
            for (MWWorld::ContainerStoreIterator it(store.begin()); it != store.end(); ++it)
            {
                if (*it == selectedItem)
                    found = true;
            }
            if (!found)
                allowSelectedItem = false;

            // if the selected item can be equipped, make sure that it actually is equipped
            std::pair<std::vector<int>, bool> slots;
            slots = MWWorld::Class::get(selectedItem).getEquipmentSlots(selectedItem);
            if (!slots.first.empty())
            {
                bool equipped = false;
                for (int i=0; i < MWWorld::InventoryStore::Slots; ++i)
                {
                    if (store.getSlot(i) != store.end() && *store.getSlot(i) == selectedItem)
                    {
                        equipped = true;
                        break;
                    }
                }

                if (!equipped)
                    allowSelectedItem = false;
            }

            if (!allowSelectedItem)
            {
                store.setSelectedEnchantItem(store.end());
                spells.setSelectedSpell("");
                mWindowManager.unsetSelectedSpell();
                selectedItem = MWWorld::Ptr();
            }
        }



        for (MWMechanics::Spells::TIterator it = spells.begin(); it != spells.end(); ++it)
        {
            spellList.push_back (it->first);
        }

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
            std::string enchantId = MWWorld::Class::get(*it).getEnchantment(*it);
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

            if (*it == selectedSpell)
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
            t->setStateSelected(*it == selectedSpell);

            // cost / success chance
            MyGUI::Button* costChance = mSpellView->createWidget<MyGUI::Button>("SpellText",
                MyGUI::IntCoord(4, mHeight, mWidth-8, spellHeight), MyGUI::Align::Left | MyGUI::Align::Top);
            std::string cost = boost::lexical_cast<std::string>(spell->mData.mCost);
            std::string chance = boost::lexical_cast<std::string>(int(MWMechanics::getSpellSuccessChance(*it, player)));
            costChance->setCaption(cost + "/" + chance);
            costChance->setTextAlign(MyGUI::Align::Right);
            costChance->setNeedMouseFocus(false);
            costChance->setStateSelected(*it == selectedSpell);


            mHeight += spellHeight;
        }


        // enchanted items
        addGroup("#{sMagicItem}", "#{sCostCharge}");

        for (std::vector<MWWorld::Ptr>::const_iterator it = items.begin(); it != items.end(); ++it)
        {
            MWWorld::Ptr item = *it;

            const ESM::Enchantment* enchant =
                esmStore.get<ESM::Enchantment>().find(MWWorld::Class::get(item).getEnchantment(item));

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
            t->setCaption(MWWorld::Class::get(item).getName(item));
            t->setTextAlign(MyGUI::Align::Left);
            t->setUserData(item);
            t->setUserString("ToolTipType", "ItemPtr");
            t->setUserString("Equipped", equipped ? "true" : "false");
            t->eventMouseButtonClick += MyGUI::newDelegate(this, &SpellWindow::onEnchantedItemSelected);
            t->eventMouseWheel += MyGUI::newDelegate(this, &SpellWindow::onMouseWheel);
            t->setStateSelected(item == selectedItem);

            // cost / charge
            MyGUI::Button* costCharge = mSpellView->createWidget<MyGUI::Button>(equipped ? "SpellText" : "SpellTextUnequipped",
                MyGUI::IntCoord(4, mHeight, mWidth-8, spellHeight), MyGUI::Align::Left | MyGUI::Align::Top);

            std::string cost = boost::lexical_cast<std::string>(enchant->mData.mCost);
            std::string charge = boost::lexical_cast<std::string>(enchant->mData.mCharge); /// \todo track current charge
            if (enchant->mData.mType == ESM::Enchantment::CastOnce)
            {
                // this is Morrowind behaviour
                cost = "100";
                charge = "100";
            }

            costCharge->setCaption(cost + "/" + charge);
            costCharge->setTextAlign(MyGUI::Align::Right);
            costCharge->setNeedMouseFocus(false);
            costCharge->setStateSelected(item == selectedItem);

            mHeight += spellHeight;
        }

        mSpellView->setCanvasSize(mSpellView->getWidth(), std::max(mSpellView->getHeight(), mHeight));
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
        }

        mHeight += 24;
    }

    void SpellWindow::onWindowResize(MyGUI::Window* _sender)
    {
        updateSpells();
    }

    void SpellWindow::onEnchantedItemSelected(MyGUI::Widget* _sender)
    {
        MWWorld::Ptr player = MWBase::Environment::get().getWorld()->getPlayer().getPlayer();
        MWMechanics::CreatureStats& stats = MWWorld::Class::get(player).getCreatureStats(player);
        MWWorld::InventoryStore& store = MWWorld::Class::get(player).getInventoryStore(player);
        MWMechanics::Spells& spells = stats.getSpells();
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
            && !MWWorld::Class::get(item).getEquipmentSlots(item).first.empty())
        {
            // Note: can't use Class::use here because enchanted scrolls for example would then open the scroll window instead of equipping

            MWWorld::ActionEquip action(item);
            action.execute (MWBase::Environment::get().getWorld ()->getPlayer ().getPlayer ());

            // since we changed equipping status, update the inventory window
            mWindowManager.getInventoryWindow()->drawItems();
        }

        store.setSelectedEnchantItem(it);
        spells.setSelectedSpell("");
        mWindowManager.setSelectedEnchantItem(item, 100); /// \todo track charge %

        updateSpells();
    }

    void SpellWindow::onSpellSelected(MyGUI::Widget* _sender)
    {
        std::string spellId = _sender->getUserString("Spell");
        MWWorld::Ptr player = MWBase::Environment::get().getWorld()->getPlayer().getPlayer();
        MWMechanics::CreatureStats& stats = MWWorld::Class::get(player).getCreatureStats(player);
        MWWorld::InventoryStore& store = MWWorld::Class::get(player).getInventoryStore(player);
        MWMechanics::Spells& spells = stats.getSpells();

        if (MyGUI::InputManager::getInstance().isShiftPressed())
        {
            // delete spell, if allowed
            const ESM::Spell* spell =
                MWBase::Environment::get().getWorld()->getStore().get<ESM::Spell>().find(spellId);

            if (spell->mData.mFlags & ESM::Spell::F_Always
                || spell->mData.mType == ESM::Spell::ST_Power)
            {
                mWindowManager.messageBox("#{sDeleteSpellError}", std::vector<std::string>());
            }
            else
            {
                // ask for confirmation
                mSpellToDelete = spellId;
                ConfirmationDialog* dialog = mWindowManager.getConfirmationDialog();
                std::string question = mWindowManager.getGameSettingString("sQuestionDeleteSpell", "Delete %s?");
                question = boost::str(boost::format(question) % spell->mName);
                dialog->open(question);
                dialog->eventOkClicked.clear();
                dialog->eventOkClicked += MyGUI::newDelegate(this, &SpellWindow::onDeleteSpellAccept);
                dialog->eventCancelClicked.clear();
            }
        }
        else
        {
            spells.setSelectedSpell(spellId);
            store.setSelectedEnchantItem(store.end());
            mWindowManager.setSelectedSpell(spellId, int(MWMechanics::getSpellSuccessChance(spellId, player)));
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
        MWWorld::Ptr player = MWBase::Environment::get().getWorld()->getPlayer().getPlayer();
        MWMechanics::CreatureStats& stats = MWWorld::Class::get(player).getCreatureStats(player);
        MWMechanics::Spells& spells = stats.getSpells();

        if (spells.getSelectedSpell() == mSpellToDelete)
        {
            spells.setSelectedSpell("");
            mWindowManager.unsetSelectedSpell();
        }

        spells.remove(mSpellToDelete);

        updateSpells();
    }
}

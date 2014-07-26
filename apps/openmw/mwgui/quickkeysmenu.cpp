#include "quickkeysmenu.hpp"

#include <boost/lexical_cast.hpp>

#include <components/esm/quickkeys.hpp>

#include "../mwworld/inventorystore.hpp"
#include "../mwworld/class.hpp"

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"

#include "../mwmechanics/spellcasting.hpp"
#include "../mwmechanics/spells.hpp"
#include "../mwmechanics/creaturestats.hpp"

#include "../mwgui/inventorywindow.hpp"
#include "../mwgui/bookwindow.hpp"
#include "../mwgui/scrollwindow.hpp"

#include "windowmanagerimp.hpp"
#include "itemselection.hpp"

#include "spellwindow.hpp"

#include "itemwidget.hpp"
#include "sortfilteritemmodel.hpp"


namespace MWGui
{

    QuickKeysMenu::QuickKeysMenu()
        : WindowBase("openmw_quickkeys_menu.layout")
        , mAssignDialog(0)
        , mItemSelectionDialog(0)
        , mMagicSelectionDialog(0)
        , mSelectedIndex(-1)
    {
        getWidget(mOkButton, "OKButton");
        getWidget(mInstructionLabel, "InstructionLabel");

        mMainWidget->setSize(mMainWidget->getWidth(),
                             mMainWidget->getHeight() + (mInstructionLabel->getTextSize().height - mInstructionLabel->getHeight()));

        mOkButton->eventMouseButtonClick += MyGUI::newDelegate(this, &QuickKeysMenu::onOkButtonClicked);
        center();


        for (int i = 0; i < 10; ++i)
        {
            ItemWidget* button;
            getWidget(button, "QuickKey" + boost::lexical_cast<std::string>(i+1));

            button->eventMouseButtonClick += MyGUI::newDelegate(this, &QuickKeysMenu::onQuickKeyButtonClicked);

            mQuickKeyButtons.push_back(button);

            mAssigned.push_back(Type_Unassigned);

            unassign(button, i);
        }
    }

    void QuickKeysMenu::exit()
    {
        MWBase::Environment::get().getWindowManager()->removeGuiMode (MWGui::GM_QuickKeysMenu);
    }

    void QuickKeysMenu::clear()
    {
        for (int i=0; i<10; ++i)
        {
            unassign(mQuickKeyButtons[i], i);
        }
    }

    QuickKeysMenu::~QuickKeysMenu()
    {
        delete mAssignDialog;
        delete mItemSelectionDialog;
        delete mMagicSelectionDialog;
    }

    void QuickKeysMenu::unassign(ItemWidget* key, int index)
    {
        key->clearUserStrings();
        key->setItem(MWWorld::Ptr());
        while (key->getChildCount()) // Destroy number label
            MyGUI::Gui::getInstance().destroyWidget(key->getChildAt(0));

        mAssigned[index] = Type_Unassigned;

        MyGUI::TextBox* textBox = key->createWidgetReal<MyGUI::TextBox>("SandText", MyGUI::FloatCoord(0,0,1,1), MyGUI::Align::Default);
        textBox->setTextAlign (MyGUI::Align::Center);
        textBox->setCaption (boost::lexical_cast<std::string>(index+1));
        textBox->setNeedMouseFocus (false);
    }

    void QuickKeysMenu::onQuickKeyButtonClicked(MyGUI::Widget* sender)
    {
        int index = -1;
        for (int i = 0; i < 10; ++i)
        {
            if (sender == mQuickKeyButtons[i] || sender->getParent () == mQuickKeyButtons[i])
            {
                index = i;
                break;
            }
        }
        assert(index != -1);
        mSelectedIndex = index;

        {
            // open assign dialog
            if (!mAssignDialog)
                mAssignDialog = new QuickKeysMenuAssign(this);
            mAssignDialog->setVisible (true);
        }
    }

    void QuickKeysMenu::onOkButtonClicked (MyGUI::Widget *sender)
    {
        MWBase::Environment::get().getWindowManager()->removeGuiMode(GM_QuickKeysMenu);
    }


    void QuickKeysMenu::onItemButtonClicked(MyGUI::Widget* sender)
    {
        if (!mItemSelectionDialog )
        {
            mItemSelectionDialog = new ItemSelectionDialog("#{sQuickMenu6}");
            mItemSelectionDialog->eventItemSelected += MyGUI::newDelegate(this, &QuickKeysMenu::onAssignItem);
            mItemSelectionDialog->eventDialogCanceled += MyGUI::newDelegate(this, &QuickKeysMenu::onAssignItemCancel);
        }
        mItemSelectionDialog->setVisible(true);
        mItemSelectionDialog->openContainer(MWBase::Environment::get().getWorld()->getPlayerPtr());
        mItemSelectionDialog->setFilter(SortFilterItemModel::Filter_OnlyUsableItems);

        mAssignDialog->setVisible (false);
    }

    void QuickKeysMenu::onMagicButtonClicked(MyGUI::Widget* sender)
    {
        if (!mMagicSelectionDialog )
        {
            mMagicSelectionDialog = new MagicSelectionDialog(this);
        }
        mMagicSelectionDialog->setVisible(true);

        mAssignDialog->setVisible (false);
    }

    void QuickKeysMenu::onUnassignButtonClicked(MyGUI::Widget* sender)
    {
        unassign(mQuickKeyButtons[mSelectedIndex], mSelectedIndex);
        mAssignDialog->setVisible (false);
    }

    void QuickKeysMenu::onCancelButtonClicked(MyGUI::Widget* sender)
    {
        mAssignDialog->setVisible (false);
    }

    void QuickKeysMenu::onAssignItem(MWWorld::Ptr item)
    {
        assert (mSelectedIndex >= 0);
        ItemWidget* button = mQuickKeyButtons[mSelectedIndex];
        while (button->getChildCount()) // Destroy number label
            MyGUI::Gui::getInstance().destroyWidget(button->getChildAt(0));

        mAssigned[mSelectedIndex] = Type_Item;

        button->setItem(item, ItemWidget::Barter);
        button->setUserString ("ToolTipType", "ItemPtr");
        button->setUserData(item);

        if (mItemSelectionDialog)
            mItemSelectionDialog->setVisible(false);
    }

    void QuickKeysMenu::onAssignItemCancel()
    {
        mItemSelectionDialog->setVisible(false);
    }

    void QuickKeysMenu::onAssignMagicItem (MWWorld::Ptr item)
    {
        assert (mSelectedIndex >= 0);
        ItemWidget* button = mQuickKeyButtons[mSelectedIndex];
        while (button->getChildCount()) // Destroy number label
            MyGUI::Gui::getInstance().destroyWidget(button->getChildAt(0));

        mAssigned[mSelectedIndex] = Type_MagicItem;

        button->setFrame("textures\\menu_icon_select_magic_magic.dds", MyGUI::IntCoord(2, 2, 40, 40));
        button->setIcon(item);

        button->setUserString ("ToolTipType", "ItemPtr");
        button->setUserData(item);

        if (mMagicSelectionDialog)
            mMagicSelectionDialog->setVisible(false);
    }

    void QuickKeysMenu::onAssignMagic (const std::string& spellId)
    {
        assert (mSelectedIndex >= 0);
        ItemWidget* button = mQuickKeyButtons[mSelectedIndex];
        while (button->getChildCount()) // Destroy number label
            MyGUI::Gui::getInstance().destroyWidget(button->getChildAt(0));

        mAssigned[mSelectedIndex] = Type_Magic;

        button->setItem(MWWorld::Ptr());
        button->setUserString ("ToolTipType", "Spell");
        button->setUserString ("Spell", spellId);

        const MWWorld::ESMStore &esmStore =
            MWBase::Environment::get().getWorld()->getStore();

        // use the icon of the first effect
        const ESM::Spell* spell = esmStore.get<ESM::Spell>().find(spellId);

        const ESM::MagicEffect* effect =
            esmStore.get<ESM::MagicEffect>().find(spell->mEffects.mList.front().mEffectID);

        std::string path = effect->mIcon;
        int slashPos = path.find("\\");
        path.insert(slashPos+1, "b_");
        path = std::string("icons\\") + path;
        int pos = path.rfind(".");
        path.erase(pos);
        path.append(".dds");

        button->setFrame("textures\\menu_icon_select_magic.dds", MyGUI::IntCoord(2, 2, 40, 40));
        button->setIcon(path);

        if (mMagicSelectionDialog)
            mMagicSelectionDialog->setVisible(false);
    }

    void QuickKeysMenu::onAssignMagicCancel ()
    {
        mMagicSelectionDialog->setVisible(false);
    }

    void QuickKeysMenu::activateQuickKey(int index)
    {
        assert (index-1 >= 0);
        ItemWidget* button = mQuickKeyButtons[index-1];

        QuickKeyType type = mAssigned[index-1];

        MWWorld::Ptr player = MWBase::Environment::get().getWorld()->getPlayerPtr();
        MWWorld::InventoryStore& store = player.getClass().getInventoryStore(player);

        if (type == Type_Item || type == Type_MagicItem)
        {
            MWWorld::Ptr item = *button->getUserData<MWWorld::Ptr>();
            // make sure the item is available
            if (item.getRefData ().getCount() < 1)
            {
                // Try searching for a compatible replacement
                std::string id = item.getCellRef().getRefId();

                for (MWWorld::ContainerStoreIterator it = store.begin(); it != store.end(); ++it)
                {
                    if (Misc::StringUtils::ciEqual(it->getCellRef().getRefId(), id))
                    {
                        item = *it;
                        button->setUserData(item);
                        break;
                    }
                }

                if (item.getRefData().getCount() < 1)
                {
                    // No replacement was found
                    MWBase::Environment::get().getWindowManager ()->messageBox (
                                "#{sQuickMenu5} " + item.getClass().getName(item));
                    return;
                }
            }
        }

        if (type == Type_Magic)
        {
            std::string spellId = button->getUserString("Spell");

            // Make sure the player still has this spell
            MWMechanics::CreatureStats& stats = player.getClass().getCreatureStats(player);
            MWMechanics::Spells& spells = stats.getSpells();
            if (!spells.hasSpell(spellId))
                return;
            store.setSelectedEnchantItem(store.end());
            MWBase::Environment::get().getWindowManager()->setSelectedSpell(spellId, int(MWMechanics::getSpellSuccessChance(spellId, player)));
        }
        else if (type == Type_Item)
        {
            MWWorld::Ptr item = *button->getUserData<MWWorld::Ptr>();

            MWBase::Environment::get().getWindowManager()->getInventoryWindow()->useItem(item);
        }
        else if (type == Type_MagicItem)
        {
            MWWorld::Ptr item = *button->getUserData<MWWorld::Ptr>();

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

            // equip, if it can be equipped
            if (!item.getClass().getEquipmentSlots(item).first.empty())
            {
                MWBase::Environment::get().getWindowManager()->getInventoryWindow()->useItem(item);
            }

            store.setSelectedEnchantItem(it);
        }
    }

    // ---------------------------------------------------------------------------------------------------------

    QuickKeysMenuAssign::QuickKeysMenuAssign (QuickKeysMenu* parent)
        : WindowModal("openmw_quickkeys_menu_assign.layout")
        , mParent(parent)
    {
        getWidget(mLabel, "Label");
        getWidget(mItemButton, "ItemButton");
        getWidget(mMagicButton, "MagicButton");
        getWidget(mUnassignButton, "UnassignButton");
        getWidget(mCancelButton, "CancelButton");

        mItemButton->eventMouseButtonClick += MyGUI::newDelegate(mParent, &QuickKeysMenu::onItemButtonClicked);
        mMagicButton->eventMouseButtonClick += MyGUI::newDelegate(mParent, &QuickKeysMenu::onMagicButtonClicked);
        mUnassignButton->eventMouseButtonClick += MyGUI::newDelegate(mParent, &QuickKeysMenu::onUnassignButtonClicked);
        mCancelButton->eventMouseButtonClick += MyGUI::newDelegate(mParent, &QuickKeysMenu::onCancelButtonClicked);


        int maxWidth = mItemButton->getTextSize ().width + 24;
        maxWidth = std::max(maxWidth, mMagicButton->getTextSize ().width + 24);
        maxWidth = std::max(maxWidth, mUnassignButton->getTextSize ().width + 24);
        maxWidth = std::max(maxWidth, mCancelButton->getTextSize ().width + 24);

        mMainWidget->setSize(maxWidth + 24, mMainWidget->getHeight());
        mLabel->setSize(maxWidth, mLabel->getHeight());

        mItemButton->setCoord((maxWidth - mItemButton->getTextSize().width-24)/2 + 8,
                              mItemButton->getTop(),
                              mItemButton->getTextSize().width + 24,
                              mItemButton->getHeight());
        mMagicButton->setCoord((maxWidth - mMagicButton->getTextSize().width-24)/2 + 8,
                              mMagicButton->getTop(),
                              mMagicButton->getTextSize().width + 24,
                              mMagicButton->getHeight());
        mUnassignButton->setCoord((maxWidth - mUnassignButton->getTextSize().width-24)/2 + 8,
                              mUnassignButton->getTop(),
                              mUnassignButton->getTextSize().width + 24,
                              mUnassignButton->getHeight());
        mCancelButton->setCoord((maxWidth - mCancelButton->getTextSize().width-24)/2 + 8,
                              mCancelButton->getTop(),
                              mCancelButton->getTextSize().width + 24,
                              mCancelButton->getHeight());

        center();
    }

    void QuickKeysMenuAssign::exit()
    {
        setVisible(false);
    }

    void QuickKeysMenu::write(ESM::ESMWriter &writer)
    {
        writer.startRecord(ESM::REC_KEYS);

        ESM::QuickKeys keys;

        for (int i=0; i<10; ++i)
        {
            ItemWidget* button = mQuickKeyButtons[i];

            int type = mAssigned[i];

            ESM::QuickKeys::QuickKey key;
            key.mType = type;

            switch (type)
            {
                case Type_Unassigned:
                    break;
                case Type_Item:
                case Type_MagicItem:
                {
                    MWWorld::Ptr item = *button->getUserData<MWWorld::Ptr>();
                    key.mId = item.getCellRef().getRefId();
                    break;
                }
                case Type_Magic:
                    std::string spellId = button->getUserString("Spell");
                    key.mId = spellId;
                    break;
            }

            keys.mKeys.push_back(key);
        }

        keys.save(writer);

        writer.endRecord(ESM::REC_KEYS);
    }

    void QuickKeysMenu::readRecord(ESM::ESMReader &reader, int32_t type)
    {
        if (type != ESM::REC_KEYS)
            return;

        ESM::QuickKeys keys;
        keys.load(reader);

        int i=0;
        for (std::vector<ESM::QuickKeys::QuickKey>::const_iterator it = keys.mKeys.begin(); it != keys.mKeys.end(); ++it)
        {
            if (i >= 10)
                return;

            mSelectedIndex = i;
            int keyType = it->mType;
            std::string id = it->mId;
            ItemWidget* button = mQuickKeyButtons[i];

            switch (keyType)
            {
            case Type_Magic:
                onAssignMagic(id);
                break;
            case Type_Item:
            case Type_MagicItem:
            {
                // Find the item by id
                MWWorld::Ptr player = MWBase::Environment::get().getWorld()->getPlayerPtr();
                MWWorld::InventoryStore& store = player.getClass().getInventoryStore(player);
                MWWorld::Ptr item;
                for (MWWorld::ContainerStoreIterator it = store.begin(); it != store.end(); ++it)
                {
                    if (Misc::StringUtils::ciEqual(it->getCellRef().getRefId(), id))
                    {
                        if (item.isEmpty() ||
                            // Prefer the stack with the lowest remaining uses
                                !item.getClass().hasItemHealth(*it) ||
                                it->getClass().getItemHealth(*it) < item.getClass().getItemHealth(item))
                        {
                            item = *it;
                        }
                    }
                }

                if (item.isEmpty())
                    unassign(button, i);
                else
                {
                    if (keyType == Type_Item)
                        onAssignItem(item);
                    else if (keyType == Type_MagicItem)
                        onAssignMagicItem(item);
                }

                break;
            }
            case Type_Unassigned:
                unassign(button, i);
                break;
            }

            ++i;
        }
    }

    // ---------------------------------------------------------------------------------------------------------

    MagicSelectionDialog::MagicSelectionDialog(QuickKeysMenu* parent)
        : WindowModal("openmw_magicselection_dialog.layout")
        , mParent(parent)
        , mWidth(0)
        , mHeight(0)
    {
        getWidget(mCancelButton, "CancelButton");
        getWidget(mMagicList, "MagicList");
        mCancelButton->eventMouseButtonClick += MyGUI::newDelegate(this, &MagicSelectionDialog::onCancelButtonClicked);

        center();
    }

    void MagicSelectionDialog::onCancelButtonClicked (MyGUI::Widget *sender)
    {
        exit();
    }

    void MagicSelectionDialog::exit()
    {
        mParent->onAssignMagicCancel();
    }

    void MagicSelectionDialog::open ()
    {
        WindowModal::open();

        while (mMagicList->getChildCount ())
            MyGUI::Gui::getInstance ().destroyWidget (mMagicList->getChildAt (0));

        mHeight = 0;

        const int spellHeight = 18;

        MWWorld::Ptr player = MWBase::Environment::get().getWorld()->getPlayerPtr();
        MWWorld::InventoryStore& store = player.getClass().getInventoryStore(player);
        MWMechanics::CreatureStats& stats = player.getClass().getCreatureStats(player);
        MWMechanics::Spells& spells = stats.getSpells();

        /// \todo lots of copy&pasted code from SpellWindow

        // retrieve powers & spells, sort by name
        std::vector<std::string> spellList;

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

        // retrieve usable magic items & sort
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
        bool scrollVisible = height > mMagicList->getHeight();
        mWidth = mMagicList->getWidth() - scrollVisible * 18;


        // powers
        addGroup("#{sPowers}", "");

        for (std::vector<std::string>::const_iterator it = powers.begin(); it != powers.end(); ++it)
        {
            const ESM::Spell* spell = esmStore.get<ESM::Spell>().find(*it);
            MyGUI::Button* t = mMagicList->createWidget<MyGUI::Button>("SpellText",
                MyGUI::IntCoord(4, mHeight, mWidth-8, spellHeight), MyGUI::Align::Left | MyGUI::Align::Top);
            t->setCaption(spell->mName);
            t->setTextAlign(MyGUI::Align::Left);
            t->setUserString("ToolTipType", "Spell");
            t->setUserString("Spell", *it);
            t->eventMouseWheel += MyGUI::newDelegate(this, &MagicSelectionDialog::onMouseWheel);
            t->eventMouseButtonClick += MyGUI::newDelegate(this, &MagicSelectionDialog::onSpellSelected);

            mHeight += spellHeight;
        }

        // other spells
        addGroup("#{sSpells}", "");
        for (std::vector<std::string>::const_iterator it = spellList.begin(); it != spellList.end(); ++it)
        {
            const ESM::Spell* spell = esmStore.get<ESM::Spell>().find(*it);
            MyGUI::Button* t = mMagicList->createWidget<MyGUI::Button>("SpellText",
                MyGUI::IntCoord(4, mHeight, mWidth-8, spellHeight), MyGUI::Align::Left | MyGUI::Align::Top);
            t->setCaption(spell->mName);
            t->setTextAlign(MyGUI::Align::Left);
            t->setUserString("ToolTipType", "Spell");
            t->setUserString("Spell", *it);
            t->eventMouseWheel += MyGUI::newDelegate(this, &MagicSelectionDialog::onMouseWheel);
            t->eventMouseButtonClick += MyGUI::newDelegate(this, &MagicSelectionDialog::onSpellSelected);

            mHeight += spellHeight;
        }


        // enchanted items
        addGroup("#{sMagicItem}", "");

        for (std::vector<MWWorld::Ptr>::const_iterator it = items.begin(); it != items.end(); ++it)
        {
            MWWorld::Ptr item = *it;

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

            MyGUI::Button* t = mMagicList->createWidget<MyGUI::Button>(equipped ? "SpellText" : "SpellTextUnequipped",
                MyGUI::IntCoord(4, mHeight, mWidth-8, spellHeight), MyGUI::Align::Left | MyGUI::Align::Top);
            t->setCaption(item.getClass().getName(item));
            t->setTextAlign(MyGUI::Align::Left);
            t->setUserData(item);
            t->setUserString("ToolTipType", "ItemPtr");
            t->eventMouseButtonClick += MyGUI::newDelegate(this, &MagicSelectionDialog::onEnchantedItemSelected);
            t->eventMouseWheel += MyGUI::newDelegate(this, &MagicSelectionDialog::onMouseWheel);

            mHeight += spellHeight;
        }

        // Canvas size must be expressed with VScroll disabled, otherwise MyGUI would expand the scroll area when the scrollbar is hidden
        mMagicList->setVisibleVScroll(false);
        mMagicList->setCanvasSize (mWidth, std::max(mMagicList->getHeight(), mHeight));
        mMagicList->setVisibleVScroll(true);
    }

    void MagicSelectionDialog::addGroup(const std::string &label, const std::string& label2)
    {
        if (mMagicList->getChildCount() > 0)
        {
            MyGUI::ImageBox* separator = mMagicList->createWidget<MyGUI::ImageBox>("MW_HLine",
                MyGUI::IntCoord(4, mHeight, mWidth-8, 18),
                MyGUI::Align::Left | MyGUI::Align::Top);
            separator->setNeedMouseFocus(false);
            mHeight += 18;
        }

        MyGUI::TextBox* groupWidget = mMagicList->createWidget<MyGUI::TextBox>("SandBrightText",
            MyGUI::IntCoord(0, mHeight, mWidth, 24),
            MyGUI::Align::Left | MyGUI::Align::Top | MyGUI::Align::HStretch);
        groupWidget->setCaptionWithReplacing(label);
        groupWidget->setTextAlign(MyGUI::Align::Left);
        groupWidget->setNeedMouseFocus(false);

        if (label2 != "")
        {
            MyGUI::TextBox* groupWidget2 = mMagicList->createWidget<MyGUI::TextBox>("SandBrightText",
                MyGUI::IntCoord(0, mHeight, mWidth-4, 24),
                MyGUI::Align::Left | MyGUI::Align::Top);
            groupWidget2->setCaptionWithReplacing(label2);
            groupWidget2->setTextAlign(MyGUI::Align::Right);
            groupWidget2->setNeedMouseFocus(false);
        }

        mHeight += 24;
    }


    void MagicSelectionDialog::onMouseWheel(MyGUI::Widget* _sender, int _rel)
    {
        if (mMagicList->getViewOffset().top + _rel*0.3 > 0)
            mMagicList->setViewOffset(MyGUI::IntPoint(0, 0));
        else
            mMagicList->setViewOffset(MyGUI::IntPoint(0, mMagicList->getViewOffset().top + _rel*0.3));
    }

    void MagicSelectionDialog::onEnchantedItemSelected(MyGUI::Widget* _sender)
    {
        MWWorld::Ptr item = *_sender->getUserData<MWWorld::Ptr>();

        mParent->onAssignMagicItem (item);
    }

    void MagicSelectionDialog::onSpellSelected(MyGUI::Widget* _sender)
    {
        mParent->onAssignMagic (_sender->getUserString("Spell"));
    }

    int MagicSelectionDialog::estimateHeight(int numSpells) const
    {
        int height = 0;
        height += 24 * 3 + 18 * 2; // group headings
        height += numSpells * 18;
        return height;
    }

}

#include "quickkeysmenu.hpp"

#include <MyGUI_EditBox.h>
#include <MyGUI_Button.h>
#include <MyGUI_Gui.h>
#include <MyGUI_ImageBox.h>

#include <components/esm/esmwriter.hpp>
#include <components/esm/quickkeys.hpp>

#include "../mwworld/inventorystore.hpp"
#include "../mwworld/class.hpp"
#include "../mwworld/player.hpp"
#include "../mwworld/esmstore.hpp"

#include "../mwbase/environment.hpp"
#include "../mwbase/mechanicsmanager.hpp"
#include "../mwbase/world.hpp"
#include "../mwbase/windowmanager.hpp"

#include "../mwmechanics/spellcasting.hpp"
#include "../mwmechanics/creaturestats.hpp"
#include "../mwmechanics/actorutil.hpp"

#include "itemselection.hpp"
#include "spellview.hpp"
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
        , mActivatedIndex(-1)
    {
        getWidget(mOkButton, "OKButton");
        getWidget(mInstructionLabel, "InstructionLabel");

        mMainWidget->setSize(mMainWidget->getWidth(),
                             mMainWidget->getHeight() +
                             (mInstructionLabel->getTextSize().height - mInstructionLabel->getHeight()));

        mOkButton->eventMouseButtonClick += MyGUI::newDelegate(this, &QuickKeysMenu::onOkButtonClicked);
        center();

        mKey = std::vector<struct keyData>(10);

        for (int i = 0; i < 10; ++i)
        {
            mKey[i].index = i;
            getWidget(mKey[i].button, "QuickKey" + MyGUI::utility::toString(i+1));
            mKey[i].button->eventMouseButtonClick += MyGUI::newDelegate(this, &QuickKeysMenu::onQuickKeyButtonClicked);

            unassign(&mKey[i]);
        }
    }

    void QuickKeysMenu::clear()
    {
        mActivatedIndex = -1;

        for (int i=0; i<10; ++i)
        {
            unassign(&mKey[i]);
        }
    }

    QuickKeysMenu::~QuickKeysMenu()
    {
        delete mAssignDialog;
        delete mItemSelectionDialog;
        delete mMagicSelectionDialog;
    }

    void QuickKeysMenu::onOpen()
    {
        WindowBase::onOpen();

        MWWorld::Ptr player = MWMechanics::getPlayer();
        MWWorld::InventoryStore& store = player.getClass().getInventoryStore(player);

        // Check if quick keys are still valid
        for (int i=0; i<10; ++i)
        {
            switch (mKey[i].type)
            {
                case Type_Unassigned:
                case Type_HandToHand:
                case Type_Magic:
                    break;
                case Type_Item:
                case Type_MagicItem:
                {
                    MWWorld::Ptr item = *mKey[i].button->getUserData<MWWorld::Ptr>();
                    // Make sure the item is available and is not broken
                    if (item.getRefData().getCount() < 1 ||
                        (item.getClass().hasItemHealth(item) &&
                        item.getClass().getItemHealth(item) <= 0))
                    {
                        // Try searching for a compatible replacement
                        std::string id = item.getCellRef().getRefId();

                        item = store.findReplacement(id);
                        mKey[i].button->setUserData(MWWorld::Ptr(item));
                        break;
                    }
                }
            }
        }
    }

    void QuickKeysMenu::unassign(struct keyData* key)
    {
        key->button->clearUserStrings();
        key->button->setItem(MWWorld::Ptr());

        while(key->button->getChildCount()) // Destroy number label
            MyGUI::Gui::getInstance().destroyWidget(key->button->getChildAt(0));

        if (key->index == 9)
        {
            key->type = Type_HandToHand;

            MyGUI::ImageBox* image = key->button->createWidget<MyGUI::ImageBox>("ImageBox",
                MyGUI::IntCoord(14, 13, 32, 32), MyGUI::Align::Default);

            image->setImageTexture("icons\\k\\stealth_handtohand.dds");
            image->setNeedMouseFocus(false);
        }
        else
        {
            key->type = Type_Unassigned;
            key->id = "";
            key->name = "";

            MyGUI::TextBox* textBox = key->button->createWidgetReal<MyGUI::TextBox>("SandText",
                MyGUI::FloatCoord(0,0,1,1), MyGUI::Align::Default);

            textBox->setTextAlign(MyGUI::Align::Center);
            textBox->setCaption(MyGUI::utility::toString(key->index + 1));
            textBox->setNeedMouseFocus(false);
        }
    }

    void QuickKeysMenu::onQuickKeyButtonClicked(MyGUI::Widget* sender)
    {
        int index = -1;
        for(int i = 0; i < 10; ++i)
        {
            if(sender == mKey[i].button || sender->getParent() == mKey[i].button)
            {
                index = i;
                break;
            }
        }
        assert(index != -1);
        mSelectedIndex = index;

        // open assign dialog
        if(!mAssignDialog)
            mAssignDialog = new QuickKeysMenuAssign(this);

        mAssignDialog->setVisible(true);
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
        mItemSelectionDialog->openContainer(MWMechanics::getPlayer());
        mItemSelectionDialog->setFilter(SortFilterItemModel::Filter_OnlyUsableItems);

        mAssignDialog->setVisible (false);
    }

    void QuickKeysMenu::onMagicButtonClicked(MyGUI::Widget* sender)
    {
        if(!mMagicSelectionDialog)
        {
            mMagicSelectionDialog = new MagicSelectionDialog(this);
        }
        mMagicSelectionDialog->setVisible(true);

        mAssignDialog->setVisible(false);
    }

    void QuickKeysMenu::onUnassignButtonClicked(MyGUI::Widget* sender)
    {
        unassign(&mKey[mSelectedIndex]);
        mAssignDialog->setVisible(false);
    }

    void QuickKeysMenu::onCancelButtonClicked(MyGUI::Widget* sender)
    {
        mAssignDialog->setVisible(false);
    }

    void QuickKeysMenu::onAssignItem(MWWorld::Ptr item)
    {
        assert(mSelectedIndex >= 0);

        while(mKey[mSelectedIndex].button->getChildCount()) // Destroy number label
            MyGUI::Gui::getInstance().destroyWidget(mKey[mSelectedIndex].button->getChildAt(0));

        mKey[mSelectedIndex].type = Type_Item;
        mKey[mSelectedIndex].id = item.getCellRef().getRefId();
        mKey[mSelectedIndex].name = item.getClass().getName(item);

        mKey[mSelectedIndex].button->setItem(item, ItemWidget::Barter);
        mKey[mSelectedIndex].button->setUserString("ToolTipType", "ItemPtr");
        mKey[mSelectedIndex].button->setUserData(item);

        if(mItemSelectionDialog)
            mItemSelectionDialog->setVisible(false);
    }

    void QuickKeysMenu::onAssignItemCancel()
    {
        mItemSelectionDialog->setVisible(false);
    }

    void QuickKeysMenu::onAssignMagicItem(MWWorld::Ptr item)
    {
        assert(mSelectedIndex >= 0);

        while(mKey[mSelectedIndex].button->getChildCount()) // Destroy number label
            MyGUI::Gui::getInstance().destroyWidget(mKey[mSelectedIndex].button->getChildAt(0));

        mKey[mSelectedIndex].type = Type_MagicItem;

        mKey[mSelectedIndex].button->setFrame("textures\\menu_icon_select_magic_magic.dds", MyGUI::IntCoord(2, 2, 40, 40));
        mKey[mSelectedIndex].button->setIcon(item);

        mKey[mSelectedIndex].button->setUserString ("ToolTipType", "ItemPtr");
        mKey[mSelectedIndex].button->setUserData(MWWorld::Ptr(item));

        if(mMagicSelectionDialog)
            mMagicSelectionDialog->setVisible(false);
    }

    void QuickKeysMenu::onAssignMagic(const std::string& spellId)
    {
        assert(mSelectedIndex >= 0);
        ItemWidget* button = mKey[mSelectedIndex].button;
        while(mKey[mSelectedIndex].button->getChildCount()) // Destroy number label
            MyGUI::Gui::getInstance().destroyWidget(button->getChildAt(0));

        mKey[mSelectedIndex].type = Type_Magic;

        mKey[mSelectedIndex].button->setItem(MWWorld::Ptr());
        mKey[mSelectedIndex].button->setUserString("ToolTipType", "Spell");
        mKey[mSelectedIndex].button->setUserString("Spell", spellId);

        const MWWorld::ESMStore &esmStore =
            MWBase::Environment::get().getWorld()->getStore();

        // use the icon of the first effect
        const ESM::Spell* spell = esmStore.get<ESM::Spell>().find(spellId);

        const ESM::MagicEffect* effect =
            esmStore.get<ESM::MagicEffect>().find(spell->mEffects.mList.front().mEffectID);

        std::string path = effect->mIcon;
        int slashPos = path.rfind('\\');
        path.insert(slashPos+1, "b_");
        path = MWBase::Environment::get().getWindowManager()->correctIconPath(path);

        button->setFrame("textures\\menu_icon_select_magic.dds", MyGUI::IntCoord(2, 2, 40, 40));
        button->setIcon(path);

        if (mMagicSelectionDialog)
            mMagicSelectionDialog->setVisible(false);
    }

    void QuickKeysMenu::onAssignMagicCancel ()
    {
        mMagicSelectionDialog->setVisible(false);
    }

    void QuickKeysMenu::updateActivatedQuickKey()
    {
        // there is no delayed action, nothing to do.
        if (mActivatedIndex < 0)
            return;

        activateQuickKey(mActivatedIndex);
    }

    void QuickKeysMenu::activateQuickKey(int index)
    {
        assert(index-1 >= 0);
        ItemWidget* button = mKey[index-1].button;
        QuickKeyType type = mKey[index-1].type;

        MWWorld::Ptr player = MWMechanics::getPlayer();
        MWWorld::InventoryStore& store = player.getClass().getInventoryStore(player);
        const MWMechanics::CreatureStats &playerStats = player.getClass().getCreatureStats(player);

        // Delay action executing,
        // if player is busy for now (casting a spell, attacking someone, etc.)
        bool isDelayNeeded = MWBase::Environment::get().getMechanicsManager()->isAttackingOrSpell(player)
                || playerStats.getKnockedDown()
                || playerStats.getHitRecovery();

        bool isReturnNeeded = playerStats.isParalyzed() || playerStats.isDead();
        if (isReturnNeeded && type != Type_Item)
        {
            return;
        }

        if (isDelayNeeded && type != Type_Item)
        {
            mActivatedIndex = index;
            return;
        }
        else
            mActivatedIndex = -1;

        if (type == Type_Item || type == Type_MagicItem)
        {
            MWWorld::Ptr item = *button->getUserData<MWWorld::Ptr>();

            MWWorld::ContainerStoreIterator it = store.begin();
            for (; it != store.end(); ++it)
            {
                if (*it == item)
                    break;
            }
            if (it == store.end())
                item = nullptr;

            // check the item is available and not broken
            if (!item || item.getRefData().getCount() < 1 ||
                (item.getClass().hasItemHealth(item) && item.getClass().getItemHealth(item) <= 0))
            {
                item = store.findReplacement(mKey[index-1].id);
                if (!item || item.getRefData().getCount() < 1)
                {
                    MWBase::Environment::get().getWindowManager()->messageBox(
                        "#{sQuickMenu5} " + mKey[index-1].name);

                    return;
                }
            }

            if (type == Type_Item)
            {
                bool isWeapon = item.getTypeName() == typeid(ESM::Weapon).name();
                bool isTool = item.getTypeName() == typeid(ESM::Probe).name() ||
                    item.getTypeName() == typeid(ESM::Lockpick).name();

                // delay weapon switching if player is busy
                if (isDelayNeeded && (isWeapon || isTool))
                {
                    mActivatedIndex = index;
                    return;
                }

                // disable weapon switching if player is dead or paralyzed
                if (isReturnNeeded && (isWeapon || isTool))
                {
                    return;
                }

                MWBase::Environment::get().getWindowManager()->useItem(item);
                MWWorld::ConstContainerStoreIterator rightHand = store.getSlot(MWWorld::InventoryStore::Slot_CarriedRight);
                // change draw state only if the item is in player's right hand
                if (rightHand != store.end() && item == *rightHand)
                {
                    MWBase::Environment::get().getWorld()->getPlayer().setDrawState(MWMechanics::DrawState_Weapon);
                }
            }
            else if (type == Type_MagicItem)
            {
                // equip, if it can be equipped
                if (!item.getClass().getEquipmentSlots(item).first.empty())
                {
                    MWBase::Environment::get().getWindowManager()->useItem(item);

                    // make sure that item was successfully equipped
                    if (!store.isEquipped(item))
                        return;
                }

                store.setSelectedEnchantItem(it);
                MWBase::Environment::get().getWorld()->getPlayer().setDrawState(MWMechanics::DrawState_Spell);
            }
        }
        else if (type == Type_Magic)
        {
            std::string spellId = button->getUserString("Spell");

            // Make sure the player still has this spell
            MWMechanics::CreatureStats& stats = player.getClass().getCreatureStats(player);
            MWMechanics::Spells& spells = stats.getSpells();
            if (!spells.hasSpell(spellId))
            {
                const ESM::Spell* spell = MWBase::Environment::get().getWorld()->getStore().get<ESM::Spell>().find(spellId);
                MWBase::Environment::get().getWindowManager()->messageBox (
                            "#{sQuickMenu5} " + spell->mName);
                return;
            }
            store.setSelectedEnchantItem(store.end());
            MWBase::Environment::get().getWindowManager()->setSelectedSpell(spellId, int(MWMechanics::getSpellSuccessChance(spellId, player)));
            MWBase::Environment::get().getWorld()->getPlayer().setDrawState(MWMechanics::DrawState_Spell);
        }
        else if (type == Type_HandToHand)
        {
            store.unequipSlot(MWWorld::InventoryStore::Slot_CarriedRight, player);
            MWBase::Environment::get().getWorld()->getPlayer().setDrawState(MWMechanics::DrawState_Weapon);
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


        int maxWidth = mLabel->getTextSize ().width + 24;
        maxWidth = std::max(maxWidth, mItemButton->getTextSize ().width + 24);
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

    void QuickKeysMenu::write(ESM::ESMWriter &writer)
    {
        writer.startRecord(ESM::REC_KEYS);

        ESM::QuickKeys keys;

        for (int i=0; i<10; ++i)
        {
            ItemWidget* button = mKey[i].button;

            int type = mKey[i].type;

            ESM::QuickKeys::QuickKey key;
            key.mType = type;

            switch (type)
            {
                case Type_Unassigned:
                case Type_HandToHand:
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

    void QuickKeysMenu::readRecord(ESM::ESMReader &reader, uint32_t type)
    {
        if(type != ESM::REC_KEYS)
            return;

        ESM::QuickKeys keys;
        keys.load(reader);

        MWWorld::Ptr player = MWMechanics::getPlayer();
        MWWorld::InventoryStore& store = player.getClass().getInventoryStore(player);

        int i=0;
        for(std::vector<ESM::QuickKeys::QuickKey>::const_iterator it = keys.mKeys.begin(); it != keys.mKeys.end(); ++it)
        {
            if(i >= 10)
                return;

            mSelectedIndex = i;
            int keyType = it->mType;
            std::string id = it->mId;

            switch (keyType)
            {
            case Type_Magic:
                if(MWBase::Environment::get().getWorld()->getStore().get<ESM::Spell>().search(id))
                    onAssignMagic(id);
                break;
            case Type_Item:
            case Type_MagicItem:
            {
                // Find the item by id
                MWWorld::Ptr item = store.findReplacement(id);

                if(item.isEmpty())
                    unassign(&mKey[i]);
                else
                {
                    if(keyType == Type_Item)
                        onAssignItem(item);
                    else if(keyType == Type_MagicItem)
                        onAssignMagicItem(item);
                }

                break;
            }
            case Type_Unassigned:
            case Type_HandToHand:
                unassign(&mKey[i]);
                break;
            }

            ++i;
        }
    }

    // ---------------------------------------------------------------------------------------------------------

    MagicSelectionDialog::MagicSelectionDialog(QuickKeysMenu* parent)
        : WindowModal("openmw_magicselection_dialog.layout")
        , mParent(parent)
    {
        getWidget(mCancelButton, "CancelButton");
        getWidget(mMagicList, "MagicList");
        mCancelButton->eventMouseButtonClick += MyGUI::newDelegate(this, &MagicSelectionDialog::onCancelButtonClicked);

        mMagicList->setShowCostColumn(false);
        mMagicList->setHighlightSelected(false);
        mMagicList->eventSpellClicked += MyGUI::newDelegate(this, &MagicSelectionDialog::onModelIndexSelected);

        center();
    }

    void MagicSelectionDialog::onCancelButtonClicked (MyGUI::Widget *sender)
    {
        exit();
    }

    bool MagicSelectionDialog::exit()
    {
        mParent->onAssignMagicCancel();
        return true;
    }

    void MagicSelectionDialog::onOpen ()
    {
        WindowModal::onOpen();

        mMagicList->setModel(new SpellModel(MWMechanics::getPlayer()));
        mMagicList->resetScrollbars();
    }

    void MagicSelectionDialog::onModelIndexSelected(SpellModel::ModelIndex index)
    {
        const Spell& spell = mMagicList->getModel()->getItem(index);
        if (spell.mType == Spell::Type_EnchantedItem)
            mParent->onAssignMagicItem(spell.mItem);
        else
            mParent->onAssignMagic(spell.mId);
    }

}

#include "quickkeysmenu.hpp"

#include <MyGUI_EditBox.h>
#include <MyGUI_Button.h>
#include <MyGUI_Gui.h>

#include <components/esm/quickkeys.hpp>
#include <components/misc/resourcehelpers.hpp>

#include "../mwworld/inventorystore.hpp"
#include "../mwworld/class.hpp"
#include "../mwworld/player.hpp"
#include "../mwworld/esmstore.hpp"

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"

#include "../mwmechanics/spellcasting.hpp"
#include "../mwmechanics/creaturestats.hpp"

#include "../mwgui/inventorywindow.hpp"
#include "../mwgui/bookwindow.hpp"
#include "../mwgui/scrollwindow.hpp"

#include "windowmanagerimp.hpp"
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
            getWidget(button, "QuickKey" + MyGUI::utility::toString(i+1));

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

        if (index == 9)
        {
            mAssigned[index] = Type_HandToHand;

            MyGUI::ImageBox* image = key->createWidget<MyGUI::ImageBox>("ImageBox",
                MyGUI::IntCoord(14, 13, 32, 32), MyGUI::Align::Default);
            image->setImageTexture("icons\\k\\stealth_handtohand.dds");
            image->setNeedMouseFocus(false);
        }
        else
        {
            mAssigned[index] = Type_Unassigned;

            MyGUI::TextBox* textBox = key->createWidgetReal<MyGUI::TextBox>("SandText", MyGUI::FloatCoord(0,0,1,1), MyGUI::Align::Default);
            textBox->setTextAlign (MyGUI::Align::Center);
            textBox->setCaption (MyGUI::utility::toString(index+1));
            textBox->setNeedMouseFocus (false);
        }
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
        int slashPos = path.rfind('\\');
        path.insert(slashPos+1, "b_");
        path = Misc::ResourceHelpers::correctIconPath(path);

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
            MWBase::Environment::get().getWorld()->getPlayer().setDrawState(MWMechanics::DrawState_Spell);
        }
        else if (type == Type_Item)
        {
            MWWorld::Ptr item = *button->getUserData<MWWorld::Ptr>();
            MWBase::Environment::get().getWindowManager()->getInventoryWindow()->useItem(item);
            MWWorld::ContainerStoreIterator rightHand = store.getSlot(MWWorld::InventoryStore::Slot_CarriedRight);
            // change draw state only if the item is in player's right hand
            if (rightHand != store.end() && item == *rightHand)
            {
                MWBase::Environment::get().getWorld()->getPlayer().setDrawState(MWMechanics::DrawState_Weapon);
            }
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

                // make sure that item was successfully equipped
                if (!store.isEquipped(item))
                    return;
            }

            store.setSelectedEnchantItem(it);
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
            case Type_HandToHand:
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

    void MagicSelectionDialog::exit()
    {
        mParent->onAssignMagicCancel();
    }

    void MagicSelectionDialog::open ()
    {
        WindowModal::open();

        mMagicList->setModel(new SpellModel(MWBase::Environment::get().getWorld()->getPlayerPtr()));
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

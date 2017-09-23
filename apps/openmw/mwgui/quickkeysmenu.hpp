#ifndef MWGUI_QUICKKEYS_H
#define MWGUI_QUICKKEYS_H

#include "../mwworld/ptr.hpp"

#include "windowbase.hpp"

#include "spellmodel.hpp"

namespace MWGui
{

    class QuickKeysMenuAssign;
    class ItemSelectionDialog;
    class MagicSelectionDialog;
    class ItemWidget;
    class SpellView;

    class QuickKeysMenu : public WindowBase
    {
    public:
        QuickKeysMenu();
        ~QuickKeysMenu();

        void onResChange(int, int) { center(); }

        void onItemButtonClicked(MyGUI::Widget* sender);
        void onMagicButtonClicked(MyGUI::Widget* sender);
        void onUnassignButtonClicked(MyGUI::Widget* sender);
        void onCancelButtonClicked(MyGUI::Widget* sender);

        void onAssignItem (MWWorld::Ptr item);
        void onAssignItemCancel ();
        void onAssignMagicItem (MWWorld::Ptr item);
        void onAssignMagic (const std::string& spellId);
        void onAssignMagicCancel ();

        void activateQuickKey(int index);
        void updateActivatedQuickKey();

        /// @note This enum is serialized, so don't move the items around!
        enum QuickKeyType
        {
            Type_Item,
            Type_Magic,
            Type_MagicItem,
            Type_Unassigned,
            Type_HandToHand
        };

        void write (ESM::ESMWriter& writer);
        void readRecord (ESM::ESMReader& reader, uint32_t type);
        void clear();


    private:
        MyGUI::EditBox* mInstructionLabel;
        MyGUI::Button* mOkButton;

        std::vector<ItemWidget*> mQuickKeyButtons;
        std::vector<QuickKeyType> mAssigned;

        QuickKeysMenuAssign* mAssignDialog;
        ItemSelectionDialog* mItemSelectionDialog;
        MagicSelectionDialog* mMagicSelectionDialog;

        int mSelectedIndex;
        int mActivatedIndex;

        void onQuickKeyButtonClicked(MyGUI::Widget* sender);
        void onOkButtonClicked(MyGUI::Widget* sender);

        void unassign(ItemWidget* key, int index);
    };

    class QuickKeysMenuAssign : public WindowModal
    {
    public:
        QuickKeysMenuAssign(QuickKeysMenu* parent);

    private:
        MyGUI::TextBox* mLabel;
        MyGUI::Button* mItemButton;
        MyGUI::Button* mMagicButton;
        MyGUI::Button* mUnassignButton;
        MyGUI::Button* mCancelButton;

        QuickKeysMenu* mParent;
    };

    class MagicSelectionDialog : public WindowModal
    {
    public:
        MagicSelectionDialog(QuickKeysMenu* parent);

        virtual void onOpen();
        virtual bool exit();

    private:
        MyGUI::Button* mCancelButton;
        SpellView* mMagicList;

        QuickKeysMenu* mParent;

        void onCancelButtonClicked (MyGUI::Widget* sender);
        void onModelIndexSelected(SpellModel::ModelIndex index);
    };
}


#endif

#ifndef MWGUI_QUICKKEYS_H
#define MWGUI_QUICKKEYS_H

#include "../mwworld/ptr.hpp"

#include "windowbase.hpp"

namespace MWGui
{

    class QuickKeysMenuAssign;
    class ItemSelectionDialog;
    class MagicSelectionDialog;

    class QuickKeysMenu : public WindowBase
    {
    public:
        QuickKeysMenu();
        ~QuickKeysMenu();


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

        enum QuickKeyType
        {
            Type_Item,
            Type_Magic,
            Type_MagicItem,
            Type_Unassigned
        };


    private:
        MyGUI::EditBox* mInstructionLabel;
        MyGUI::Button* mOkButton;

        std::vector<MyGUI::Button*> mQuickKeyButtons;

        QuickKeysMenuAssign* mAssignDialog;
        ItemSelectionDialog* mItemSelectionDialog;
        MagicSelectionDialog* mMagicSelectionDialog;

        int mSelectedIndex;


        void onQuickKeyButtonClicked(MyGUI::Widget* sender);
        void onOkButtonClicked(MyGUI::Widget* sender);

        void unassign(MyGUI::Widget* key, int index);
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

        virtual void open();

    private:
        MyGUI::Button* mCancelButton;
        MyGUI::ScrollView* mMagicList;

        int mWidth;
        int mHeight;

        QuickKeysMenu* mParent;

        void onCancelButtonClicked (MyGUI::Widget* sender);
        void onMouseWheel(MyGUI::Widget* _sender, int _rel);
        void onEnchantedItemSelected(MyGUI::Widget* _sender);
        void onSpellSelected(MyGUI::Widget* _sender);
        int estimateHeight(int numSpells) const;


        void addGroup(const std::string& label, const std::string& label2);
    };
}


#endif

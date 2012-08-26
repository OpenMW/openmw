#ifndef MWGUI_QUICKKEYS_H
#define MWGUI_QUICKKEYS_H


#include "../mwworld/ptr.hpp"

#include "window_base.hpp"

namespace MWGui
{

    class QuickKeysMenuAssign;
    class ItemSelectionDialog;
    class MagicSelectionDialog;

    class QuickKeysMenu : public WindowBase
    {
    public:
        QuickKeysMenu(MWBase::WindowManager& parWindowManager);
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
        QuickKeysMenuAssign(MWBase::WindowManager& parWindowManager, QuickKeysMenu* parent);

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
        MagicSelectionDialog(MWBase::WindowManager& parWindowManager, QuickKeysMenu* parent);

    private:
        MyGUI::Button* mCancelButton;

        QuickKeysMenu* mParent;

        void onCancelButtonClicked (MyGUI::Widget* sender);
    };
}


#endif

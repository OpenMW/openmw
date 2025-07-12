#ifndef MWGUI_QUICKKEYS_H
#define MWGUI_QUICKKEYS_H

#include <memory>

#include "components/esm3/quickkeys.hpp"

#include "itemselection.hpp"
#include "spellmodel.hpp"
#include "windowbase.hpp"

namespace MWGui
{

    class QuickKeysMenuAssign;
    class MagicSelectionDialog;
    class ItemWidget;
    class SpellView;

    class QuickKeysMenu : public WindowBase
    {
    public:
        QuickKeysMenu();

        void onResChange(int, int) override { center(); }

        void onItemButtonClicked(MyGUI::Widget* sender);
        void onMagicButtonClicked(MyGUI::Widget* sender);
        void onUnassignButtonClicked(MyGUI::Widget* sender);
        void onCancelButtonClicked(MyGUI::Widget* sender);

        void onAssignItem(MWWorld::Ptr item);
        void onAssignItemCancel();
        void onAssignMagicItem(MWWorld::Ptr item);
        void onAssignMagic(const ESM::RefId& spellId);
        void onAssignMagicCancel();
        void onOpen() override;
        void onClose() override;

        void activateQuickKey(int index);
        void updateActivatedQuickKey();

        void write(ESM::ESMWriter& writer);
        void readRecord(ESM::ESMReader& reader, uint32_t type);
        void clear() override;

        std::string_view getWindowIdForLua() const override { return "QuickKeys"; }

    private:
        struct keyData
        {
            int index = -1;
            ItemWidget* button = nullptr;
            ESM::QuickKeys::Type type = ESM::QuickKeys::Type::Unassigned;
            ESM::RefId id;
            std::string name;
        };

        std::vector<keyData> mKey;
        keyData* mSelected;
        keyData* mActivated;

        MyGUI::EditBox* mInstructionLabel;
        MyGUI::Button* mOkButton;

        std::unique_ptr<QuickKeysMenuAssign> mAssignDialog;
        std::unique_ptr<ItemSelectionDialog> mItemSelectionDialog;
        std::unique_ptr<MagicSelectionDialog> mMagicSelectionDialog;

        void onQuickKeyButtonClicked(MyGUI::Widget* sender);
        void onOkButtonClicked(MyGUI::Widget* sender);
        // Check if quick key is still valid
        inline void validate(int index);
        void unassign(keyData* key);

        bool onControllerButtonEvent(const SDL_ControllerButtonEvent& arg) override;
        size_t mControllerFocus;
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

        bool onControllerButtonEvent(const SDL_ControllerButtonEvent& arg) override;
        int mControllerFocus;
    };

    class MagicSelectionDialog : public WindowModal
    {
    public:
        MagicSelectionDialog(QuickKeysMenu* parent);

        void onOpen() override;
        bool exit() override;

    private:
        MyGUI::Button* mCancelButton;
        SpellView* mMagicList;

        QuickKeysMenu* mParent;

        void onCancelButtonClicked(MyGUI::Widget* sender);
        void onModelIndexSelected(SpellModel::ModelIndex index);

        bool onControllerButtonEvent(const SDL_ControllerButtonEvent& arg) override;
        int mControllerFocus;
    };
}

#endif

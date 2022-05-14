#ifndef MYGUI_POSTPROCESSOR_HUD_H
#define MYGUI_POSTPROCESSOR_HUD_H

#include "windowbase.hpp"

#include <MyGUI_Gui.h>
#include <MyGUI_ListBox.h>

#include <components/settings/shadermanager.hpp>

namespace MyGUI
{
    class ScrollView;
    class EditBox;
    class TabItem;
}
namespace Gui
{
    class AutoSizedButton;
    class AutoSizedEditBox;
}

namespace MWGui
{
    class PostProcessorHud : public WindowBase
    {
        class ListWrapper final : public MyGUI::ListBox
        {
            MYGUI_RTTI_DERIVED(ListWrapper)
        protected:
            void onKeyButtonPressed(MyGUI::KeyCode key, MyGUI::Char ch) override;
        };

    public:
        PostProcessorHud();

        void onOpen() override;

        void onClose() override;

        void updateTechniques();

        void toggleMode(Settings::ShaderManager::Mode mode);

        static void registerMyGUIComponents();

    private:

        void notifyWindowResize(MyGUI::Window* sender);

        void notifyFilterChanged(MyGUI::EditBox* sender);

        void updateConfigView(const std::string& name);

        void notifyModeToggle(MyGUI::Widget* sender);

        void notifyResetButtonClicked(MyGUI::Widget* sender);

        void notifyListChangePosition(MyGUI::ListBox* sender, size_t index);

        void notifyKeyButtonPressed(MyGUI::Widget* sender, MyGUI::KeyCode key, MyGUI::Char ch);

        void notifyActivatePressed(MyGUI::Widget* sender);

        void notifyDeactivatePressed(MyGUI::Widget* sender);

        void notifyShaderUpPressed(MyGUI::Widget* sender);

        void notifyShaderDownPressed(MyGUI::Widget* sender);

        enum class Direction
        {
            Up,
            Down
        };

        void moveShader(Direction direction);

        void toggleTechnique(bool enabled);

        void select(ListWrapper* list, size_t index);

        void layout();

        MyGUI::TabItem* mTabConfiguration;

        ListWrapper* mActiveList;
        ListWrapper* mInactiveList;

        Gui::AutoSizedButton* mButtonActivate;
        Gui::AutoSizedButton* mButtonDeactivate;
        Gui::AutoSizedButton* mButtonDown;
        Gui::AutoSizedButton* mButtonUp;

        MyGUI::ScrollView* mConfigLayout;

        MyGUI::Widget* mConfigArea;

        MyGUI::EditBox* mFilter;
        Gui::AutoSizedButton* mModeToggle;
        Gui::AutoSizedEditBox* mShaderInfo;

        std::string mOverrideHint;
    };
}

#endif

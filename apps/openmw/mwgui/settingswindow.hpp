#ifndef MWGUI_SETTINGS_H
#define MWGUI_SETTINGS_H

#include "windowbase.hpp"

namespace MWGui
{
    class WindowManager;
}

namespace MWGui
{
    class SettingsWindow : public WindowBase
    {
        public:
            SettingsWindow();

            virtual void open();

            virtual void exit();

            void updateControlsBox();

    protected:
            MyGUI::Button* mOkButton;

            // graphics
            MyGUI::ListBox* mResolutionList;
            MyGUI::Button* mFullscreenButton;
            MyGUI::Button* mVSyncButton;
            MyGUI::Button* mWindowBorderButton;
            MyGUI::Button* mFPSButton;
            MyGUI::ScrollBar* mFOVSlider;
            MyGUI::ScrollBar* mDifficultySlider;
            MyGUI::ScrollBar* mAnisotropySlider;
            MyGUI::ComboBox* mTextureFilteringButton;
            MyGUI::TextBox* mAnisotropyLabel;
            MyGUI::Widget* mAnisotropyBox;
            MyGUI::Button* mShadersButton;
            MyGUI::Button* mShaderModeButton;
            MyGUI::Button* mRefractionButton;

            MyGUI::Button* mShadowsEnabledButton;
            MyGUI::ComboBox* mShadowsTextureSize;

            // controls
            MyGUI::ScrollView* mControlsBox;
            MyGUI::Button* mResetControlsButton;
            MyGUI::Button* mKeyboardSwitch;
            MyGUI::Button* mControllerSwitch;
            bool mKeyboardMode; //if true, setting up the keyboard. Otherwise, it's controller

            void onOkButtonClicked(MyGUI::Widget* _sender);
            void onFpsToggled(MyGUI::Widget* _sender);
            void onTextureFilteringChanged(MyGUI::ComboBox* _sender, size_t pos);
            void onSliderChangePosition(MyGUI::ScrollBar* scroller, size_t pos);
            void onButtonToggled(MyGUI::Widget* _sender);
            void onResolutionSelected(MyGUI::ListBox* _sender, size_t index);
            void onResolutionAccept();
            void onResolutionCancel();

            void onShaderModeToggled(MyGUI::Widget* _sender);
            void onShadowTextureSizeChanged(MyGUI::ComboBox* _sender, size_t pos);

            void onRebindAction(MyGUI::Widget* _sender);
            void onInputTabMouseWheel(MyGUI::Widget* _sender, int _rel);
            void onResetDefaultBindings(MyGUI::Widget* _sender);
            void onResetDefaultBindingsAccept ();
            void onKeyboardSwitchClicked(MyGUI::Widget* _sender);
            void onControllerSwitchClicked(MyGUI::Widget* _sender);

            void onWindowResize(MyGUI::Window* _sender);

            void apply();

            void configureWidgets(MyGUI::Widget* widget);
    };
}

#endif

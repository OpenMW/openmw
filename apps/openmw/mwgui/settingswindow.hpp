#ifndef MWGUI_SETTINGS_H
#define MWGUI_SETTINGS_H

#include "windowbase.hpp"

namespace MWGui
{
    class SettingsWindow : public WindowBase
    {
        public:
            SettingsWindow();

            void onOpen() override;

            void updateControlsBox();

            void updateLightSettings();

            void onResChange(int, int) override { center(); }

    protected:
            MyGUI::TabControl* mSettingsTab;
            MyGUI::Button* mOkButton;

            // graphics
            MyGUI::ListBox* mResolutionList;
            MyGUI::Button* mFullscreenButton;
            MyGUI::Button* mWindowBorderButton;
            MyGUI::ComboBox* mTextureFilteringButton;
            MyGUI::Widget* mAnisotropyBox;

            MyGUI::ComboBox* mWaterTextureSize;
            MyGUI::ComboBox* mWaterReflectionDetail;
            MyGUI::ComboBox* mWaterRainRippleDetail;

            MyGUI::ComboBox* mMaxLights;
            MyGUI::ComboBox* mLightingMethodButton;
            MyGUI::Button* mLightsResetButton;

            // controls
            MyGUI::ScrollView* mControlsBox;
            MyGUI::Button* mResetControlsButton;
            MyGUI::Button* mKeyboardSwitch;
            MyGUI::Button* mControllerSwitch;
            bool mKeyboardMode; //if true, setting up the keyboard. Otherwise, it's controller

            void onTabChanged(MyGUI::TabControl* _sender, size_t index);
            void onOkButtonClicked(MyGUI::Widget* _sender);
            void onTextureFilteringChanged(MyGUI::ComboBox* _sender, size_t pos);
            void onSliderChangePosition(MyGUI::ScrollBar* scroller, size_t pos);
            void onButtonToggled(MyGUI::Widget* _sender);
            void onResolutionSelected(MyGUI::ListBox* _sender, size_t index);
            void onResolutionAccept();
            void onResolutionCancel();
            void highlightCurrentResolution();

            void onWaterTextureSizeChanged(MyGUI::ComboBox* _sender, size_t pos);
            void onWaterReflectionDetailChanged(MyGUI::ComboBox* _sender, size_t pos);
            void onWaterRainRippleDetailChanged(MyGUI::ComboBox* _sender, size_t pos);

            void onLightingMethodButtonChanged(MyGUI::ComboBox* _sender, size_t pos);
            void onLightsResetButtonClicked(MyGUI::Widget* _sender);
            void onMaxLightsChanged(MyGUI::ComboBox* _sender, size_t pos);

            void onRebindAction(MyGUI::Widget* _sender);
            void onInputTabMouseWheel(MyGUI::Widget* _sender, int _rel);
            void onResetDefaultBindings(MyGUI::Widget* _sender);
            void onResetDefaultBindingsAccept ();
            void onKeyboardSwitchClicked(MyGUI::Widget* _sender);
            void onControllerSwitchClicked(MyGUI::Widget* _sender);

            void onWindowResize(MyGUI::Window* _sender);

            void apply();

            void configureWidgets(MyGUI::Widget* widget, bool init);
            void updateSliderLabel(MyGUI::ScrollBar* scroller, const std::string& value);

            void layoutControlsBox();

            void computeMinimumWindowSize();
        
        private:
            void resetScrollbars();
    };
}

#endif

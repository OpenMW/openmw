#ifndef MWGUI_SETTINGS_H
#define MWGUI_SETTINGS_H

#include <components/lua_ui/adapter.hpp>

#include "windowbase.hpp"

namespace MWGui
{
    class SettingsWindow : public WindowBase
    {
    public:
        SettingsWindow();

        void onOpen() override;

        void onClose() override;

        void onFrame(float duration) override;

        void updateControlsBox();

        void updateLightSettings();

        void updateVSyncModeSettings();

        void updateWindowModeSettings();

        void onResChange(int, int) override;

    protected:
        MyGUI::TabControl* mSettingsTab;
        MyGUI::Button* mOkButton;

        // graphics
        MyGUI::ListBox* mResolutionList;
        MyGUI::ComboBox* mWindowModeList;
        MyGUI::ComboBox* mVSyncModeList;
        MyGUI::Button* mWindowBorderButton;
        MyGUI::ComboBox* mTextureFilteringButton;

        MyGUI::Button* mWaterRefractionButton;
        MyGUI::Button* mSunlightScatteringButton;
        MyGUI::Button* mWobblyShoresButton;
        MyGUI::ComboBox* mWaterTextureSize;
        MyGUI::ComboBox* mWaterReflectionDetail;
        MyGUI::ComboBox* mWaterRainRippleDetail;

        MyGUI::ComboBox* mMaxLights;
        MyGUI::ComboBox* mLightingMethodButton;
        MyGUI::Button* mLightsResetButton;

        MyGUI::ComboBox* mPrimaryLanguage;
        MyGUI::ComboBox* mSecondaryLanguage;
        MyGUI::Button* mGmstOverridesL10n;

        MyGUI::Widget* mWindowModeHint;

        // controls
        MyGUI::ScrollView* mControlsBox;
        MyGUI::Button* mResetControlsButton;
        MyGUI::Button* mKeyboardSwitch;
        MyGUI::Button* mControllerSwitch;
        bool mKeyboardMode; // if true, setting up the keyboard. Otherwise, it's controller

        MyGUI::EditBox* mScriptFilter;
        MyGUI::ListBox* mScriptList;
        MyGUI::Widget* mScriptBox;
        MyGUI::Widget* mScriptDisabled;
        MyGUI::ScrollView* mScriptView;
        LuaUi::LuaAdapter* mScriptAdapter;
        int mCurrentPage;

        void onTabChanged(MyGUI::TabControl* _sender, size_t index);
        void onOkButtonClicked(MyGUI::Widget* _sender);
        void onTextureFilteringChanged(MyGUI::ComboBox* _sender, size_t pos);
        void onSliderChangePosition(MyGUI::ScrollBar* scroller, size_t pos);
        void onButtonToggled(MyGUI::Widget* _sender);
        void onResolutionSelected(MyGUI::ListBox* _sender, size_t index);
        void onResolutionAccept();
        void onResolutionCancel();
        void highlightCurrentResolution();

        void onRefractionButtonClicked(MyGUI::Widget* _sender);
        void onWaterTextureSizeChanged(MyGUI::ComboBox* _sender, size_t pos);
        void onWaterReflectionDetailChanged(MyGUI::ComboBox* _sender, size_t pos);
        void onWaterRainRippleDetailChanged(MyGUI::ComboBox* _sender, size_t pos);

        void onLightingMethodButtonChanged(MyGUI::ComboBox* _sender, size_t pos);
        void onLightsResetButtonClicked(MyGUI::Widget* _sender);
        void onMaxLightsChanged(MyGUI::ComboBox* _sender, size_t pos);

        void onPrimaryLanguageChanged(MyGUI::ComboBox* _sender, size_t pos) { onLanguageChanged(0, _sender, pos); }
        void onSecondaryLanguageChanged(MyGUI::ComboBox* _sender, size_t pos) { onLanguageChanged(1, _sender, pos); }
        void onLanguageChanged(size_t langPriority, MyGUI::ComboBox* _sender, size_t pos);
        void onGmstOverridesL10nChanged(MyGUI::Widget* _sender);

        void onWindowModeChanged(MyGUI::ComboBox* _sender, size_t pos);
        void onVSyncModeChanged(MyGUI::ComboBox* _sender, size_t pos);

        void onRebindAction(MyGUI::Widget* _sender);
        void onInputTabMouseWheel(MyGUI::Widget* _sender, int _rel);
        void onResetDefaultBindings(MyGUI::Widget* _sender);
        void onResetDefaultBindingsAccept();
        void onKeyboardSwitchClicked(MyGUI::Widget* _sender);
        void onControllerSwitchClicked(MyGUI::Widget* _sender);

        void onWindowResize(MyGUI::Window* _sender);

        void onScriptFilterChange(MyGUI::EditBox*);
        void onScriptListSelection(MyGUI::ListBox*, size_t index);

        void apply();

        void configureWidgets(MyGUI::Widget* widget, bool init);
        void updateSliderLabel(MyGUI::ScrollBar* scroller, const std::string& value);

        void layoutControlsBox();
        void renderScriptSettings();

        void computeMinimumWindowSize();

    private:
        void resetScrollbars();
    };
}

#endif

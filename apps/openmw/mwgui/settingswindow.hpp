#ifndef MWGUI_SETTINGS_H
#define MWGUI_SETTINGS_H

#include "window_base.hpp"

namespace MWGui
{
    class WindowManager;
}

namespace MWGui
{
    class SettingsWindow : public WindowBase
    {
        public:
            SettingsWindow(WindowManager& parWindowManager);

        private:
            static const float sFovMin = 30;
            static const float sFovMax = 140;
            static const float sViewDistMin = 2000;
            static const float sViewDistMax = 5600;

        protected:
            MyGUI::Button* mOkButton;

            MyGUI::ScrollBar* mMenuTransparencySlider;
            MyGUI::ScrollBar* mToolTipDelaySlider;

            // graphics
            MyGUI::ListBox* mResolutionList;
            MyGUI::Button* mFullscreenButton;
            MyGUI::Button* mVSyncButton;
            MyGUI::Button* mFPSButton;
            MyGUI::ScrollBar* mViewDistanceSlider;
            MyGUI::ScrollBar* mFOVSlider;
            MyGUI::ScrollBar* mAnisotropySlider;
            MyGUI::Button* mTextureFilteringButton;
            MyGUI::TextBox* mAnisotropyLabel;
            MyGUI::Widget* mAnisotropyBox;

            // audio
            MyGUI::ScrollBar* mMasterVolumeSlider;
            MyGUI::ScrollBar* mVoiceVolumeSlider;
            MyGUI::ScrollBar* mEffectsVolumeSlider;
            MyGUI::ScrollBar* mFootstepsVolumeSlider;
            MyGUI::ScrollBar* mMusicVolumeSlider;

            void onOkButtonClicked(MyGUI::Widget* _sender);
            void onFpsToggled(MyGUI::Widget* _sender);
            void onTextureFilteringToggled(MyGUI::Widget* _sender);
            void onSliderChangePosition(MyGUI::ScrollBar* scroller, size_t pos);
            void onButtonToggled(MyGUI::Widget* _sender);
            void onResolutionSelected(MyGUI::ListBox* _sender, size_t index);
            void onResolutionAccept();
            void onResolutionCancel();

            void apply();
    };
}

#endif


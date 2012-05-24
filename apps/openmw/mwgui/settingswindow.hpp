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

        protected:
            MyGUI::Button* mOkButton;
            MyGUI::ListBox* mResolutionList;
            MyGUI::Button* mFullscreenButton;
            MyGUI::ScrollBar* mMenuTransparencySlider;
            MyGUI::ScrollBar* mViewDistanceSlider;

            // audio
            MyGUI::ScrollBar* mMasterVolumeSlider;
            MyGUI::ScrollBar* mVoiceVolumeSlider;
            MyGUI::ScrollBar* mEffectsVolumeSlider;
            MyGUI::ScrollBar* mFootstepsVolumeSlider;
            MyGUI::ScrollBar* mMusicVolumeSlider;

            void onOkButtonClicked(MyGUI::Widget* _sender);
            void onSliderChangePosition(MyGUI::ScrollBar* scroller, size_t pos);
            void onButtonToggled(MyGUI::Widget* _sender);
            void onResolutionSelected(MyGUI::ListBox* _sender, size_t index);
            void onResolutionAccept();

            void apply();
    };
}

#endif


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

            void updateControlsBox();

        private:
            static int const sFovMin = 30;
            static int const sFovMax = 140;
            static int const sViewDistMin = 2000;
            static int const sViewDistMax = 5600;

        protected:
            MyGUI::Button* mOkButton;

            MyGUI::ScrollBar* mMenuTransparencySlider;
            MyGUI::ScrollBar* mToolTipDelaySlider;
            MyGUI::Button* mSubtitlesButton;
            MyGUI::Button* mCrosshairButton;
            MyGUI::Button* mBestAttackButton;

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
            MyGUI::Button* mWaterShaderButton;
            MyGUI::Button* mReflectObjectsButton;
            MyGUI::Button* mReflectActorsButton;
            MyGUI::Button* mReflectTerrainButton;
            MyGUI::Button* mShadersButton;
            MyGUI::Button* mShaderModeButton;
            MyGUI::Button* mRefractionButton;

            MyGUI::Button* mShadowsEnabledButton;
            MyGUI::Button* mShadowsLargeDistance;
            MyGUI::Button* mShadowsTextureSize;
            MyGUI::Button* mActorShadows;
            MyGUI::Button* mStaticsShadows;
            MyGUI::Button* mMiscShadows;
            MyGUI::Button* mShadowsDebug;

            // audio
            MyGUI::ScrollBar* mMasterVolumeSlider;
            MyGUI::ScrollBar* mVoiceVolumeSlider;
            MyGUI::ScrollBar* mEffectsVolumeSlider;
            MyGUI::ScrollBar* mFootstepsVolumeSlider;
            MyGUI::ScrollBar* mMusicVolumeSlider;

            // controls
            MyGUI::ScrollView* mControlsBox;
            MyGUI::Button* mResetControlsButton;
            MyGUI::Button* mInvertYButton;
            MyGUI::ScrollBar* mCameraSensitivitySlider;

            void onOkButtonClicked(MyGUI::Widget* _sender);
            void onFpsToggled(MyGUI::Widget* _sender);
            void onTextureFilteringToggled(MyGUI::Widget* _sender);
            void onSliderChangePosition(MyGUI::ScrollBar* scroller, size_t pos);
            void onButtonToggled(MyGUI::Widget* _sender);
            void onResolutionSelected(MyGUI::ListBox* _sender, size_t index);
            void onResolutionAccept();
            void onResolutionCancel();

            void onShadersToggled(MyGUI::Widget* _sender);
            void onShaderModeToggled(MyGUI::Widget* _sender);
            void onShadowTextureSize(MyGUI::Widget* _sender);

            void onRebindAction(MyGUI::Widget* _sender);
            void onInputTabMouseWheel(MyGUI::Widget* _sender, int _rel);
            void onResetDefaultBindings(MyGUI::Widget* _sender);
            void onResetDefaultBindingsAccept ();

            void apply();
    };
}

#endif

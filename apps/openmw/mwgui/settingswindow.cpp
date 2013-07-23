#include "settingswindow.hpp"

#include <OgreRoot.h>
#include <OgrePlugin.h>

#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/math/common_factor_rt.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwbase/soundmanager.hpp"
#include "../mwbase/inputmanager.hpp"
#include "../mwbase/windowmanager.hpp"

#include "confirmationdialog.hpp"

namespace
{
    std::string fpsLevelToStr(int level)
    {
        if (level == 0)
            return "#{sOff}";
        else if (level == 1)
            return "Basic";
        else
            return "Detailed";
    }

    std::string textureFilteringToStr(const std::string& val)
    {
        if (val == "none")
            return "None";
        else if (val == "anisotropic")
            return "Anisotropic";
        else if (val == "bilinear")
            return "Bilinear";
        else
            return "Trilinear";
    }

    void parseResolution (int &x, int &y, const std::string& str)
    {
        std::vector<std::string> split;
        boost::algorithm::split (split, str, boost::is_any_of("@(x"));
        assert (split.size() >= 2);
        boost::trim(split[0]);
        boost::trim(split[1]);
        x = boost::lexical_cast<int> (split[0]);
        y = boost::lexical_cast<int> (split[1]);
    }

    bool sortResolutions (std::pair<int, int> left, std::pair<int, int> right)
    {
        if (left.first == right.first)
            return left.second > right.second;
        return left.first > right.first;
    }

    std::string getAspect (int x, int y)
    {
        int gcd = boost::math::gcd (x, y);
        int xaspect = x / gcd;
        int yaspect = y / gcd;
        // special case: 8 : 5 is usually referred to as 16:10
        if (xaspect == 8 && yaspect == 5)
            return "16 : 10";
        return boost::lexical_cast<std::string>(xaspect) + " : " + boost::lexical_cast<std::string>(yaspect);
    }

    std::string hlslGlsl ()
    {
        return (Ogre::Root::getSingleton ().getRenderSystem ()->getName ().find("OpenGL") != std::string::npos) ? "glsl" : "hlsl";
    }

    bool cgAvailable ()
    {
        Ogre::Root::PluginInstanceList list = Ogre::Root::getSingleton ().getInstalledPlugins ();
        for (Ogre::Root::PluginInstanceList::const_iterator it = list.begin(); it != list.end(); ++it)
        {
            if ((*it)->getName() == "Cg Program Manager")
                return true;
        }
        return false;
    }
}

namespace MWGui
{
    SettingsWindow::SettingsWindow() :
        WindowBase("openmw_settings_window.layout")
    {
        getWidget(mOkButton, "OkButton");
        getWidget(mBestAttackButton, "BestAttackButton");
        getWidget(mSubtitlesButton, "SubtitlesButton");
        getWidget(mCrosshairButton, "CrosshairButton");
        getWidget(mResolutionList, "ResolutionList");
        getWidget(mMenuTransparencySlider, "MenuTransparencySlider");
        getWidget(mToolTipDelaySlider, "ToolTipDelaySlider");
        getWidget(mViewDistanceSlider, "ViewDistanceSlider");
        getWidget(mFullscreenButton, "FullscreenButton");
        getWidget(mVSyncButton, "VSyncButton");
        getWidget(mFPSButton, "FPSButton");
        getWidget(mFOVSlider, "FOVSlider");
        getWidget(mMasterVolumeSlider, "MasterVolume");
        getWidget(mVoiceVolumeSlider, "VoiceVolume");
        getWidget(mEffectsVolumeSlider, "EffectsVolume");
        getWidget(mFootstepsVolumeSlider, "FootstepsVolume");
        getWidget(mMusicVolumeSlider, "MusicVolume");
        getWidget(mAnisotropySlider, "AnisotropySlider");
        getWidget(mTextureFilteringButton, "TextureFilteringButton");
        getWidget(mAnisotropyLabel, "AnisotropyLabel");
        getWidget(mAnisotropyBox, "AnisotropyBox");
        getWidget(mWaterShaderButton, "WaterShaderButton");
        getWidget(mReflectObjectsButton, "ReflectObjectsButton");
        getWidget(mReflectActorsButton, "ReflectActorsButton");
        getWidget(mReflectTerrainButton, "ReflectTerrainButton");
        getWidget(mShadersButton, "ShadersButton");
        getWidget(mShaderModeButton, "ShaderModeButton");
        getWidget(mShadowsEnabledButton, "ShadowsEnabledButton");
        getWidget(mShadowsLargeDistance, "ShadowsLargeDistance");
        getWidget(mShadowsTextureSize, "ShadowsTextureSize");
        getWidget(mActorShadows, "ActorShadows");
        getWidget(mStaticsShadows, "StaticsShadows");
        getWidget(mMiscShadows, "MiscShadows");
        getWidget(mShadowsDebug, "ShadowsDebug");
        getWidget(mControlsBox, "ControlsBox");
        getWidget(mResetControlsButton, "ResetControlsButton");
        getWidget(mInvertYButton, "InvertYButton");
        getWidget(mCameraSensitivitySlider, "CameraSensitivitySlider");
        getWidget(mRefractionButton, "RefractionButton");

        mSubtitlesButton->eventMouseButtonClick += MyGUI::newDelegate(this, &SettingsWindow::onButtonToggled);
        mCrosshairButton->eventMouseButtonClick += MyGUI::newDelegate(this, &SettingsWindow::onButtonToggled);
        mBestAttackButton->eventMouseButtonClick += MyGUI::newDelegate(this, &SettingsWindow::onButtonToggled);
        mInvertYButton->eventMouseButtonClick += MyGUI::newDelegate(this, &SettingsWindow::onButtonToggled);
        mOkButton->eventMouseButtonClick += MyGUI::newDelegate(this, &SettingsWindow::onOkButtonClicked);
        mShadersButton->eventMouseButtonClick += MyGUI::newDelegate(this, &SettingsWindow::onShadersToggled);
        mShaderModeButton->eventMouseButtonClick += MyGUI::newDelegate(this, &SettingsWindow::onShaderModeToggled);
        mFullscreenButton->eventMouseButtonClick += MyGUI::newDelegate(this, &SettingsWindow::onButtonToggled);
        mWaterShaderButton->eventMouseButtonClick += MyGUI::newDelegate(this, &SettingsWindow::onButtonToggled);
        mRefractionButton->eventMouseButtonClick += MyGUI::newDelegate(this, &SettingsWindow::onButtonToggled);
        mReflectObjectsButton->eventMouseButtonClick += MyGUI::newDelegate(this, &SettingsWindow::onButtonToggled);
        mReflectTerrainButton->eventMouseButtonClick += MyGUI::newDelegate(this, &SettingsWindow::onButtonToggled);
        mReflectActorsButton->eventMouseButtonClick += MyGUI::newDelegate(this, &SettingsWindow::onButtonToggled);
        mTextureFilteringButton->eventMouseButtonClick += MyGUI::newDelegate(this, &SettingsWindow::onTextureFilteringToggled);
        mVSyncButton->eventMouseButtonClick += MyGUI::newDelegate(this, &SettingsWindow::onButtonToggled);
        mFPSButton->eventMouseButtonClick += MyGUI::newDelegate(this, &SettingsWindow::onFpsToggled);
        mMenuTransparencySlider->eventScrollChangePosition += MyGUI::newDelegate(this, &SettingsWindow::onSliderChangePosition);
        mFOVSlider->eventScrollChangePosition += MyGUI::newDelegate(this, &SettingsWindow::onSliderChangePosition);
        mToolTipDelaySlider->eventScrollChangePosition += MyGUI::newDelegate(this, &SettingsWindow::onSliderChangePosition);
        mViewDistanceSlider->eventScrollChangePosition += MyGUI::newDelegate(this, &SettingsWindow::onSliderChangePosition);
        mResolutionList->eventListChangePosition += MyGUI::newDelegate(this, &SettingsWindow::onResolutionSelected);
        mAnisotropySlider->eventScrollChangePosition += MyGUI::newDelegate(this, &SettingsWindow::onSliderChangePosition);

        mShadowsEnabledButton->eventMouseButtonClick += MyGUI::newDelegate(this, &SettingsWindow::onButtonToggled);
        mShadowsLargeDistance->eventMouseButtonClick += MyGUI::newDelegate(this, &SettingsWindow::onButtonToggled);
        mShadowsTextureSize->eventMouseButtonClick += MyGUI::newDelegate(this, &SettingsWindow::onShadowTextureSize);
        mActorShadows->eventMouseButtonClick += MyGUI::newDelegate(this, &SettingsWindow::onButtonToggled);
        mStaticsShadows->eventMouseButtonClick += MyGUI::newDelegate(this, &SettingsWindow::onButtonToggled);
        mMiscShadows->eventMouseButtonClick += MyGUI::newDelegate(this, &SettingsWindow::onButtonToggled);
        mShadowsDebug->eventMouseButtonClick += MyGUI::newDelegate(this, &SettingsWindow::onButtonToggled);

        mMasterVolumeSlider->eventScrollChangePosition += MyGUI::newDelegate(this, &SettingsWindow::onSliderChangePosition);
        mVoiceVolumeSlider->eventScrollChangePosition += MyGUI::newDelegate(this, &SettingsWindow::onSliderChangePosition);
        mEffectsVolumeSlider->eventScrollChangePosition += MyGUI::newDelegate(this, &SettingsWindow::onSliderChangePosition);
        mFootstepsVolumeSlider->eventScrollChangePosition += MyGUI::newDelegate(this, &SettingsWindow::onSliderChangePosition);
        mMusicVolumeSlider->eventScrollChangePosition += MyGUI::newDelegate(this, &SettingsWindow::onSliderChangePosition);

        center();

        mResetControlsButton->eventMouseButtonClick += MyGUI::newDelegate(this, &SettingsWindow::onResetDefaultBindings);

        // fill resolution list
        Ogre::RenderSystem* rs = Ogre::Root::getSingleton().getRenderSystem();
        Ogre::StringVector videoModes = rs->getConfigOptions()["Video Mode"].possibleValues;
        std::vector < std::pair<int, int> > resolutions;
        for (Ogre::StringVector::const_iterator it=videoModes.begin();
            it!=videoModes.end(); ++it)
        {

            int resX, resY;
            parseResolution (resX, resY, *it);
            resolutions.push_back(std::make_pair(resX, resY));
        }
        std::sort(resolutions.begin(), resolutions.end(), sortResolutions);
        for (std::vector < std::pair<int, int> >::const_iterator it=resolutions.begin();
             it!=resolutions.end(); ++it)
        {
            std::string str = boost::lexical_cast<std::string>(it->first) + " x " + boost::lexical_cast<std::string>(it->second)
                    + " (" + getAspect(it->first,it->second) + ")";

            if (mResolutionList->findItemIndexWith(str) == MyGUI::ITEM_NONE)
                mResolutionList->addItem(str);
        }

        // read settings
        int menu_transparency = (mMenuTransparencySlider->getScrollRange()-1) * Settings::Manager::getFloat("menu transparency", "GUI");
        mMenuTransparencySlider->setScrollPosition(menu_transparency);
        int tooltip_delay = (mToolTipDelaySlider->getScrollRange()-1) * Settings::Manager::getFloat("tooltip delay", "GUI");
        mToolTipDelaySlider->setScrollPosition(tooltip_delay);

        mSubtitlesButton->setCaptionWithReplacing(Settings::Manager::getBool("subtitles", "GUI") ? "#{sOn}" : "#{sOff}");
        mCrosshairButton->setCaptionWithReplacing(Settings::Manager::getBool("crosshair", "HUD") ? "#{sOn}" : "#{sOff}");
        mBestAttackButton->setCaptionWithReplacing(Settings::Manager::getBool("best attack", "Game") ? "#{sOn}" : "#{sOff}");

        float fovVal = (Settings::Manager::getFloat("field of view", "General")-sFovMin)/(sFovMax-sFovMin);
        mFOVSlider->setScrollPosition(fovVal * (mFOVSlider->getScrollRange()-1));
        MyGUI::TextBox* fovText;
        getWidget(fovText, "FovText");
        fovText->setCaption("Field of View (" + boost::lexical_cast<std::string>(int(Settings::Manager::getFloat("field of view", "General"))) + ")");

        float anisotropyVal = Settings::Manager::getInt("anisotropy", "General") / 16.0;
        mAnisotropySlider->setScrollPosition(anisotropyVal * (mAnisotropySlider->getScrollRange()-1));
        std::string tf = Settings::Manager::getString("texture filtering", "General");
        mTextureFilteringButton->setCaption(textureFilteringToStr(tf));
        mAnisotropyLabel->setCaption("Anisotropy (" + boost::lexical_cast<std::string>(Settings::Manager::getInt("anisotropy", "General")) + ")");

        float val = (Settings::Manager::getFloat("max viewing distance", "Viewing distance")-sViewDistMin)/(sViewDistMax-sViewDistMin);
        int viewdist = (mViewDistanceSlider->getScrollRange()-1) * val;
        mViewDistanceSlider->setScrollPosition(viewdist);

        mMasterVolumeSlider->setScrollPosition(Settings::Manager::getFloat("master volume", "Sound") * (mMasterVolumeSlider->getScrollRange()-1));
        mMusicVolumeSlider->setScrollPosition(Settings::Manager::getFloat("music volume", "Sound") * (mMusicVolumeSlider->getScrollRange()-1));
        mEffectsVolumeSlider->setScrollPosition(Settings::Manager::getFloat("sfx volume", "Sound") * (mEffectsVolumeSlider->getScrollRange()-1));
        mFootstepsVolumeSlider->setScrollPosition(Settings::Manager::getFloat("footsteps volume", "Sound") * (mFootstepsVolumeSlider->getScrollRange()-1));
        mVoiceVolumeSlider->setScrollPosition(Settings::Manager::getFloat("voice volume", "Sound") * (mVoiceVolumeSlider->getScrollRange()-1));

        mWaterShaderButton->setCaptionWithReplacing(Settings::Manager::getBool("shader", "Water") ? "#{sOn}" : "#{sOff}");
        mReflectObjectsButton->setCaptionWithReplacing(Settings::Manager::getBool("reflect statics", "Water") ? "#{sOn}" : "#{sOff}");
        mReflectActorsButton->setCaptionWithReplacing(Settings::Manager::getBool("reflect actors", "Water") ? "#{sOn}" : "#{sOff}");
        mReflectTerrainButton->setCaptionWithReplacing(Settings::Manager::getBool("reflect terrain", "Water") ? "#{sOn}" : "#{sOff}");

        mShadowsTextureSize->setCaption (Settings::Manager::getString ("texture size", "Shadows"));
        mShadowsLargeDistance->setCaptionWithReplacing(Settings::Manager::getBool("split", "Shadows") ? "#{sOn}" : "#{sOff}");

        mShadowsEnabledButton->setCaptionWithReplacing(Settings::Manager::getBool("enabled", "Shadows") ? "#{sOn}" : "#{sOff}");
        mActorShadows->setCaptionWithReplacing(Settings::Manager::getBool("actor shadows", "Shadows") ? "#{sOn}" : "#{sOff}");
        mStaticsShadows->setCaptionWithReplacing(Settings::Manager::getBool("statics shadows", "Shadows") ? "#{sOn}" : "#{sOff}");
        mMiscShadows->setCaptionWithReplacing(Settings::Manager::getBool("misc shadows", "Shadows") ? "#{sOn}" : "#{sOff}");
        mShadowsDebug->setCaptionWithReplacing(Settings::Manager::getBool("debug", "Shadows") ? "#{sOn}" : "#{sOff}");

        float cameraSens = (Settings::Manager::getFloat("camera sensitivity", "Input")-0.2)/(5.0-0.2);
        mCameraSensitivitySlider->setScrollPosition (cameraSens * (mCameraSensitivitySlider->getScrollRange()-1));
        mCameraSensitivitySlider->eventScrollChangePosition += MyGUI::newDelegate(this, &SettingsWindow::onSliderChangePosition);

        mInvertYButton->setCaptionWithReplacing(Settings::Manager::getBool("invert y axis", "Input") ? "#{sOn}" : "#{sOff}");

        mShadersButton->setCaptionWithReplacing (Settings::Manager::getBool("shaders", "Objects") ? "#{sOn}" : "#{sOff}");
        mShaderModeButton->setCaption (Settings::Manager::getString("shader mode", "General"));

        mRefractionButton->setCaptionWithReplacing (Settings::Manager::getBool("refraction", "Water") ? "#{sOn}" : "#{sOff}");

        if (!Settings::Manager::getBool("shaders", "Objects"))
        {
            mRefractionButton->setEnabled(false);
            mShadowsEnabledButton->setEnabled(false);
        }

        mFullscreenButton->setCaptionWithReplacing(Settings::Manager::getBool("fullscreen", "Video") ? "#{sOn}" : "#{sOff}");
        mVSyncButton->setCaptionWithReplacing(Settings::Manager::getBool("vsync", "Video") ? "#{sOn}": "#{sOff}");
        mFPSButton->setCaptionWithReplacing(fpsLevelToStr(Settings::Manager::getInt("fps", "HUD")));
    }

    void SettingsWindow::onOkButtonClicked(MyGUI::Widget* _sender)
    {
        MWBase::Environment::get().getWindowManager()->removeGuiMode(GM_Settings);
    }

    void SettingsWindow::onResolutionSelected(MyGUI::ListBox* _sender, size_t index)
    {
        if (index == MyGUI::ITEM_NONE)
            return;

        /*
        ConfirmationDialog* dialog = MWBase::Environment::get().getWindowManager()->getConfirmationDialog();
        dialog->open("#{sNotifyMessage67}");
        dialog->eventOkClicked.clear();
        dialog->eventOkClicked += MyGUI::newDelegate(this, &SettingsWindow::onResolutionAccept);
        dialog->eventCancelClicked.clear();
        dialog->eventCancelClicked += MyGUI::newDelegate(this, &SettingsWindow::onResolutionCancel);
        */
        onResolutionAccept();
    }

    void SettingsWindow::onResolutionAccept()
    {
        std::string resStr = mResolutionList->getItemNameAt(mResolutionList->getIndexSelected());
        int resX, resY;
        parseResolution (resX, resY, resStr);

        Settings::Manager::setInt("resolution x", "Video", resX);
        Settings::Manager::setInt("resolution y", "Video", resY);

        apply();

        MWBase::Environment::get().getWindowManager()->
            messageBox("New resolution will be applied after a restart", std::vector<std::string>());
    }

    void SettingsWindow::onResolutionCancel()
    {
        mResolutionList->setIndexSelected(MyGUI::ITEM_NONE);
    }

    void SettingsWindow::onShadowTextureSize(MyGUI::Widget* _sender)
    {
        std::string size = mShadowsTextureSize->getCaption();

        if (size == "512")
            size = "1024";
        else if (size == "1024")
            size = "2048";
        else if (size == "2048")
            size = "4096";
        else
            size = "512";

        mShadowsTextureSize->setCaption(size);

        Settings::Manager::setString("texture size", "Shadows", size);
        apply();
    }

    void SettingsWindow::onButtonToggled(MyGUI::Widget* _sender)
    {
        std::string on = MWBase::Environment::get().getWindowManager()->getGameSettingString("sOn", "On");
        std::string off = MWBase::Environment::get().getWindowManager()->getGameSettingString("sOff", "On");
        bool newState;
        if (_sender->castType<MyGUI::Button>()->getCaption() == on)
        {
            _sender->castType<MyGUI::Button>()->setCaption(off);
            newState = false;
        }
        else
        {
            _sender->castType<MyGUI::Button>()->setCaption(on);
            newState = true;
        }

        if (_sender == mFullscreenButton)
        {
            // check if this resolution is supported in fullscreen
            bool supported = false;
            for (unsigned int i=0; i<mResolutionList->getItemCount(); ++i)
            {
                std::string resStr = mResolutionList->getItemNameAt(i);
                int resX, resY;
                parseResolution (resX, resY, resStr);

                if (resX == Settings::Manager::getInt("resolution x", "Video")
                    && resY  == Settings::Manager::getInt("resolution y", "Video"))
                    supported = true;
            }

            if (!supported)
            {
                std::string msg = "This resolution is not supported in Fullscreen mode. Please select a resolution from the list.";
                MWBase::Environment::get().getWindowManager()->
                    messageBox(msg);
                _sender->castType<MyGUI::Button>()->setCaption(off);
            }
            else
            {
                Settings::Manager::setBool("fullscreen", "Video", newState);
                apply();
                MWBase::Environment::get().getWindowManager()->
                    messageBox("Fullscreen will be applied after a restart", std::vector<std::string>());
            }
        }
        else if (_sender == mVSyncButton)
        {
            Settings::Manager::setBool("vsync", "Video", newState);
            MWBase::Environment::get().getWindowManager()->
                messageBox("VSync will be applied after a restart", std::vector<std::string>());
        }
        else
        {
            if (_sender == mVSyncButton)
                Settings::Manager::setBool("vsync", "Video", newState);
            if (_sender == mWaterShaderButton)
                Settings::Manager::setBool("shader", "Water", newState);
            else if (_sender == mRefractionButton)
                Settings::Manager::setBool("refraction", "Water", newState);
            else if (_sender == mReflectObjectsButton)
            {
                Settings::Manager::setBool("reflect misc", "Water", newState);
                Settings::Manager::setBool("reflect statics", "Water", newState);
                Settings::Manager::setBool("reflect statics small", "Water", newState);
            }
            else if (_sender == mReflectActorsButton)
                Settings::Manager::setBool("reflect actors", "Water", newState);
            else if (_sender == mReflectTerrainButton)
                Settings::Manager::setBool("reflect terrain", "Water", newState);
            else if (_sender == mShadowsEnabledButton)
                Settings::Manager::setBool("enabled", "Shadows", newState);
            else if (_sender == mShadowsLargeDistance)
                Settings::Manager::setBool("split", "Shadows", newState);
            else if (_sender == mActorShadows)
                Settings::Manager::setBool("actor shadows", "Shadows", newState);
            else if (_sender == mStaticsShadows)
                Settings::Manager::setBool("statics shadows", "Shadows", newState);
            else if (_sender == mMiscShadows)
                Settings::Manager::setBool("misc shadows", "Shadows", newState);
            else if (_sender == mShadowsDebug)
                Settings::Manager::setBool("debug", "Shadows", newState);
            else if (_sender == mInvertYButton)
                Settings::Manager::setBool("invert y axis", "Input", newState);
            else if (_sender == mCrosshairButton)
                Settings::Manager::setBool("crosshair", "HUD", newState);
            else if (_sender == mSubtitlesButton)
                Settings::Manager::setBool("subtitles", "GUI", newState);
            else if (_sender == mBestAttackButton)
                Settings::Manager::setBool("best attack", "Game", newState);

            apply();
        }
    }

    void SettingsWindow::onShaderModeToggled(MyGUI::Widget* _sender)
    {
        std::string val = static_cast<MyGUI::Button*>(_sender)->getCaption();
        if (val == "cg")
        {
            val = hlslGlsl();
        }
        else if (cgAvailable ())
            val = "cg";

        static_cast<MyGUI::Button*>(_sender)->setCaption(val);

        Settings::Manager::setString("shader mode", "General", val);

        apply();
    }

    void SettingsWindow::onShadersToggled(MyGUI::Widget* _sender)
    {
        std::string on = MWBase::Environment::get().getWindowManager()->getGameSettingString("sOn", "On");
        std::string off = MWBase::Environment::get().getWindowManager()->getGameSettingString("sOff", "On");

        std::string val = static_cast<MyGUI::Button*>(_sender)->getCaption();
        if (val == off)
            val = on;
        else
            val = off;
        static_cast<MyGUI::Button*>(_sender)->setCaptionWithReplacing (val);

        if (val == off)
        {
            Settings::Manager::setBool("shaders", "Objects", false);

            // refraction needs shaders to display underwater fog
            mRefractionButton->setCaptionWithReplacing("#{sOff}");
            mRefractionButton->setEnabled(false);

            Settings::Manager::setBool("refraction", "Water", false);
            Settings::Manager::setBool("underwater effect", "Water", false);

            // shadows not supported
            mShadowsEnabledButton->setEnabled(false);
            mShadowsEnabledButton->setCaptionWithReplacing("#{sOff}");
            Settings::Manager::setBool("enabled", "Shadows", false);
        }
        else
        {
            Settings::Manager::setBool("shaders", "Objects", true);

            // re-enable
            mReflectObjectsButton->setEnabled(true);
            mReflectActorsButton->setEnabled(true);
            mReflectTerrainButton->setEnabled(true);
            mRefractionButton->setEnabled(true);

            mShadowsEnabledButton->setEnabled(true);
        }

        apply();
    }

    void SettingsWindow::onFpsToggled(MyGUI::Widget* _sender)
    {
        int newLevel = (Settings::Manager::getInt("fps", "HUD") + 1) % 3;
        Settings::Manager::setInt("fps", "HUD", newLevel);
        mFPSButton->setCaptionWithReplacing(fpsLevelToStr(newLevel));
        apply();
    }

    void SettingsWindow::onTextureFilteringToggled(MyGUI::Widget* _sender)
    {
        std::string current = Settings::Manager::getString("texture filtering", "General");
        std::string next;
        if (current == "none")
            next = "bilinear";
        else if (current == "bilinear")
            next = "trilinear";
        else if (current == "trilinear")
            next = "anisotropic";
        else
            next = "none";

        mTextureFilteringButton->setCaption(textureFilteringToStr(next));

        Settings::Manager::setString("texture filtering", "General", next);
        apply();
    }

    void SettingsWindow::onSliderChangePosition(MyGUI::ScrollBar* scroller, size_t pos)
    {
        float val = pos / float(scroller->getScrollRange()-1);
        if (scroller == mMenuTransparencySlider)
            Settings::Manager::setFloat("menu transparency", "GUI", val);
        else if (scroller == mToolTipDelaySlider)
            Settings::Manager::setFloat("tooltip delay", "GUI", val);
        else if (scroller == mViewDistanceSlider)
            Settings::Manager::setFloat("max viewing distance", "Viewing distance", (1-val) * sViewDistMin + val * sViewDistMax);
        else if (scroller == mFOVSlider)
        {
            MyGUI::TextBox* fovText;
            getWidget(fovText, "FovText");
            fovText->setCaption("Field of View (" + boost::lexical_cast<std::string>(int((1-val) * sFovMin + val * sFovMax)) + ")");
            Settings::Manager::setFloat("field of view", "General", (1-val) * sFovMin + val * sFovMax);
        }
        else if (scroller == mAnisotropySlider)
        {
            mAnisotropyLabel->setCaption("Anisotropy (" + boost::lexical_cast<std::string>(int(val*16)) + ")");
            Settings::Manager::setInt("anisotropy", "General", val * 16);
        }
        else if (scroller == mMasterVolumeSlider)
            Settings::Manager::setFloat("master volume", "Sound", val);
        else if (scroller == mVoiceVolumeSlider)
            Settings::Manager::setFloat("voice volume", "Sound", val);
        else if (scroller == mEffectsVolumeSlider)
            Settings::Manager::setFloat("sfx volume", "Sound", val);
        else if (scroller == mFootstepsVolumeSlider)
            Settings::Manager::setFloat("footsteps volume", "Sound", val);
        else if (scroller == mMusicVolumeSlider)
            Settings::Manager::setFloat("music volume", "Sound", val);
        else if (scroller == mCameraSensitivitySlider)
            Settings::Manager::setFloat("camera sensitivity", "Input", (1-val) * 0.2 + val * 5.f);

        apply();
    }

    void SettingsWindow::apply()
    {
        const Settings::CategorySettingVector changed = Settings::Manager::apply();
        MWBase::Environment::get().getWorld()->processChangedSettings(changed);
        MWBase::Environment::get().getSoundManager()->processChangedSettings(changed);
        MWBase::Environment::get().getWindowManager()->processChangedSettings(changed);
        MWBase::Environment::get().getInputManager()->processChangedSettings(changed);
    }

    void SettingsWindow::updateControlsBox()
    {
        while (mControlsBox->getChildCount())
            MyGUI::Gui::getInstance().destroyWidget(mControlsBox->getChildAt(0));

        MWBase::Environment::get().getWindowManager ()->removeStaticMessageBox();

        std::vector<int> actions = MWBase::Environment::get().getInputManager()->getActionSorting ();

        const int h = 18;
        const int w = mControlsBox->getWidth() - 28;
        int curH = 0;
        for (std::vector<int>::const_iterator it = actions.begin(); it != actions.end(); ++it)
        {
            std::string desc = MWBase::Environment::get().getInputManager()->getActionDescription (*it);
            if (desc == "")
                continue;

            std::string binding = MWBase::Environment::get().getInputManager()->getActionBindingName (*it);

            MyGUI::TextBox* leftText = mControlsBox->createWidget<MyGUI::TextBox>("SandText", MyGUI::IntCoord(0,curH,w,h), MyGUI::Align::Default);
            leftText->setCaptionWithReplacing(desc);

            MyGUI::Button* rightText = mControlsBox->createWidget<MyGUI::Button>("SandTextButton", MyGUI::IntCoord(0,curH,w,h), MyGUI::Align::Default);
            rightText->setCaptionWithReplacing(binding);
            rightText->setTextAlign (MyGUI::Align::Right);
            rightText->setUserData(*it); // save the action id for callbacks
            rightText->eventMouseButtonClick += MyGUI::newDelegate(this, &SettingsWindow::onRebindAction);
            rightText->eventMouseWheel += MyGUI::newDelegate(this, &SettingsWindow::onInputTabMouseWheel);
            curH += h;
        }

        mControlsBox->setCanvasSize (mControlsBox->getWidth(), std::max(curH, mControlsBox->getHeight()));
    }

    void SettingsWindow::onRebindAction(MyGUI::Widget* _sender)
    {
        int actionId = *_sender->getUserData<int>();

        static_cast<MyGUI::Button*>(_sender)->setCaptionWithReplacing("#{sNone}");

        MWBase::Environment::get().getWindowManager ()->staticMessageBox ("#{sControlsMenu3}");
        MWBase::Environment::get().getWindowManager ()->disallowMouse();

        MWBase::Environment::get().getInputManager ()->enableDetectingBindingMode (actionId);

    }

    void SettingsWindow::onInputTabMouseWheel(MyGUI::Widget* _sender, int _rel)
    {
        if (mControlsBox->getViewOffset().top + _rel*0.3 > 0)
            mControlsBox->setViewOffset(MyGUI::IntPoint(0, 0));
        else
            mControlsBox->setViewOffset(MyGUI::IntPoint(0, mControlsBox->getViewOffset().top + _rel*0.3));
    }

    void SettingsWindow::onResetDefaultBindings(MyGUI::Widget* _sender)
    {
        ConfirmationDialog* dialog = MWBase::Environment::get().getWindowManager()->getConfirmationDialog();
        dialog->open("#{sNotifyMessage66}");
        dialog->eventOkClicked.clear();
        dialog->eventOkClicked += MyGUI::newDelegate(this, &SettingsWindow::onResetDefaultBindingsAccept);
        dialog->eventCancelClicked.clear();
    }

    void SettingsWindow::onResetDefaultBindingsAccept()
    {
        MWBase::Environment::get().getInputManager ()->resetToDefaultBindings ();
        updateControlsBox ();
    }

    void SettingsWindow::open()
    {
        updateControlsBox ();
    }
}

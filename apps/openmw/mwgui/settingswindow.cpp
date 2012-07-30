#include "settingswindow.hpp"

#include <OgreRoot.h>
#include <OgreRenderSystem.h>
#include <OgreString.h>

#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/math/common_factor_rt.hpp>

#include <components/settings/settings.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"

#include "../mwrender/renderingmanager.hpp"

#include "../mwsound/soundmanager.hpp"

#include "../mwinput/inputmanager.hpp"

#include "window_manager.hpp"
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

    bool hasGLSL ()
    {
        return (Ogre::Root::getSingleton ().getRenderSystem ()->getName ().find("OpenGL") != std::string::npos);
    }
}

namespace MWGui
{
    SettingsWindow::SettingsWindow(WindowManager& parWindowManager) :
        WindowBase("openmw_settings_window.layout", parWindowManager)
    {
        getWidget(mOkButton, "OkButton");
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
        getWidget(mShadowsEnabledButton, "ShadowsEnabledButton");
        getWidget(mShadowsLargeDistance, "ShadowsLargeDistance");
        getWidget(mShadowsTextureSize, "ShadowsTextureSize");
        getWidget(mActorShadows, "ActorShadows");
        getWidget(mStaticsShadows, "StaticsShadows");
        getWidget(mMiscShadows, "MiscShadows");
        getWidget(mShadowsDebug, "ShadowsDebug");
        getWidget(mUnderwaterButton, "UnderwaterButton");

        mOkButton->eventMouseButtonClick += MyGUI::newDelegate(this, &SettingsWindow::onOkButtonClicked);
        mUnderwaterButton->eventMouseButtonClick += MyGUI::newDelegate(this, &SettingsWindow::onButtonToggled);
        mShadersButton->eventMouseButtonClick += MyGUI::newDelegate(this, &SettingsWindow::onShadersToggled);
        mFullscreenButton->eventMouseButtonClick += MyGUI::newDelegate(this, &SettingsWindow::onButtonToggled);
        mWaterShaderButton->eventMouseButtonClick += MyGUI::newDelegate(this, &SettingsWindow::onButtonToggled);
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

        int okSize = mOkButton->getTextSize().width + 24;
        mOkButton->setCoord(mMainWidget->getWidth()-16-okSize, mOkButton->getTop(),
                            okSize, mOkButton->getHeight());

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
        mUnderwaterButton->setCaptionWithReplacing(Settings::Manager::getBool("underwater effect", "Water") ? "#{sOn}" : "#{sOff}");

        mShadowsTextureSize->setCaption (Settings::Manager::getString ("texture size", "Shadows"));
        mShadowsLargeDistance->setCaptionWithReplacing(Settings::Manager::getBool("split", "Shadows") ? "#{sOn}" : "#{sOff}");
        mShadowsEnabledButton->setCaptionWithReplacing(Settings::Manager::getBool("enabled", "Shadows") ? "#{sOn}" : "#{sOff}");
        mActorShadows->setCaptionWithReplacing(Settings::Manager::getBool("actor shadows", "Shadows") ? "#{sOn}" : "#{sOff}");
        mStaticsShadows->setCaptionWithReplacing(Settings::Manager::getBool("statics shadows", "Shadows") ? "#{sOn}" : "#{sOff}");
        mMiscShadows->setCaptionWithReplacing(Settings::Manager::getBool("misc shadows", "Shadows") ? "#{sOn}" : "#{sOff}");
        mShadowsDebug->setCaptionWithReplacing(Settings::Manager::getBool("debug", "Shadows") ? "#{sOn}" : "#{sOff}");

        std::string shaders;
        if (!Settings::Manager::getBool("shaders", "Objects"))
            shaders = "off";
        else
        {
            shaders = Settings::Manager::getString("shader mode", "General");
        }
        mShadersButton->setCaption (shaders);

        if (!MWRender::RenderingManager::waterShaderSupported())
        {
            mWaterShaderButton->setEnabled(false);
            mReflectObjectsButton->setEnabled(false);
            mReflectActorsButton->setEnabled(false);
            mReflectTerrainButton->setEnabled(false);
        }

        if (shaders == "off")
        {
            mUnderwaterButton->setEnabled (false);
            mShadowsEnabledButton->setEnabled(false);
        }

        mFullscreenButton->setCaptionWithReplacing(Settings::Manager::getBool("fullscreen", "Video") ? "#{sOn}" : "#{sOff}");
        mVSyncButton->setCaptionWithReplacing(Settings::Manager::getBool("vsync", "Video") ? "#{sOn}": "#{sOff}");
        mFPSButton->setCaptionWithReplacing(fpsLevelToStr(Settings::Manager::getInt("fps", "HUD")));
    }

    void SettingsWindow::onOkButtonClicked(MyGUI::Widget* _sender)
    {
        mWindowManager.removeGuiMode(GM_Settings);
    }

    void SettingsWindow::onResolutionSelected(MyGUI::ListBox* _sender, size_t index)
    {
        if (index == MyGUI::ITEM_NONE)
            return;

        ConfirmationDialog* dialog = mWindowManager.getConfirmationDialog();
        dialog->open("#{sNotifyMessage67}");
        dialog->eventOkClicked.clear();
        dialog->eventOkClicked += MyGUI::newDelegate(this, &SettingsWindow::onResolutionAccept);
        dialog->eventCancelClicked.clear();
    }

    void SettingsWindow::onResolutionAccept()
    {
        std::string resStr = mResolutionList->getItemNameAt(mResolutionList->getIndexSelected());
        int resX, resY;
        parseResolution (resX, resY, resStr);

        Settings::Manager::setInt("resolution x", "Video", resX);
        Settings::Manager::setInt("resolution y", "Video", resY);

        apply();
        mResolutionList->setIndexSelected(MyGUI::ITEM_NONE);
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
        std::string on = mWindowManager.getGameSettingString("sOn", "On");
        std::string off = mWindowManager.getGameSettingString("sOff", "On");
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
                    messageBox(msg, std::vector<std::string>());
                _sender->castType<MyGUI::Button>()->setCaption(off);
            }
            else
            {
                Settings::Manager::setBool("fullscreen", "Video", newState);
                apply();
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
            if (_sender == mWaterShaderButton)
                Settings::Manager::setBool("shader", "Water", newState);
            else if (_sender == mUnderwaterButton)
            {
                Settings::Manager::setBool("underwater effect", "Water", newState);
            }
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

            apply();
        }
    }

    void SettingsWindow::onShadersToggled(MyGUI::Widget* _sender)
    {
        std::string val = static_cast<MyGUI::Button*>(_sender)->getCaption();
        if (val == "off")
        {
            if (hasGLSL ())
                val = "glsl";
            else
                val = "cg";
        }
        else if (val == "glsl")
            val = "cg";
        else
            val = "off";

        static_cast<MyGUI::Button*>(_sender)->setCaption(val);

        if (val == "off")
        {
            Settings::Manager::setBool("shaders", "Objects", false);

            // water shader not supported with object shaders off
            mWaterShaderButton->setCaptionWithReplacing("#{sOff}");
            mWaterShaderButton->setEnabled(false);
            mReflectObjectsButton->setEnabled(false);
            mReflectActorsButton->setEnabled(false);
            mReflectTerrainButton->setEnabled(false);
            mUnderwaterButton->setEnabled(false);
            Settings::Manager::setBool("shader", "Water", false);
            Settings::Manager::setBool("underwater effect", "Water", false);

            // shadows not supported
            mShadowsEnabledButton->setEnabled(false);
            mShadowsEnabledButton->setCaptionWithReplacing("#{sOff}");
            Settings::Manager::setBool("enabled", "Shadows", false);
        }
        else
        {
            Settings::Manager::setBool("shaders", "Objects", true);
            Settings::Manager::setString("shader mode", "General", val);

            // re-enable
            if (MWRender::RenderingManager::waterShaderSupported())
            {
                mWaterShaderButton->setEnabled(true);
                mReflectObjectsButton->setEnabled(true);
                mReflectActorsButton->setEnabled(true);
                mReflectTerrainButton->setEnabled(true);
            }
            mUnderwaterButton->setEnabled(true);
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
}

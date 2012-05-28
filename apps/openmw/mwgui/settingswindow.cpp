#include "settingswindow.hpp"

#include <OgreRoot.h>
#include <OgreRenderSystem.h>
#include <OgreString.h>

#include <boost/lexical_cast.hpp>

#include <components/settings/settings.hpp>

#include "../mwbase/environment.hpp"
#include "../mwworld/world.hpp"
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
}

namespace MWGui
{
    SettingsWindow::SettingsWindow(WindowManager& parWindowManager) :
        WindowBase("openmw_settings_window_layout.xml", parWindowManager)
    {
        getWidget(mOkButton, "OkButton");
        getWidget(mResolutionList, "ResolutionList");
        getWidget(mMenuTransparencySlider, "MenuTransparencySlider");
        getWidget(mToolTipDelaySlider, "ToolTipDelaySlider");
        getWidget(mViewDistanceSlider, "ViewDistanceSlider");
        getWidget(mFullscreenButton, "FullscreenButton");
        getWidget(mVSyncButton, "VSyncButton");
        getWidget(mFPSButton, "FPSButton");
        getWidget(mMasterVolumeSlider, "MasterVolume");
        getWidget(mVoiceVolumeSlider, "VoiceVolume");
        getWidget(mEffectsVolumeSlider, "EffectsVolume");
        getWidget(mFootstepsVolumeSlider, "FootstepsVolume");
        getWidget(mMusicVolumeSlider, "MusicVolume");

        mOkButton->eventMouseButtonClick += MyGUI::newDelegate(this, &SettingsWindow::onOkButtonClicked);
        mFullscreenButton->eventMouseButtonClick += MyGUI::newDelegate(this, &SettingsWindow::onButtonToggled);
        mVSyncButton->eventMouseButtonClick += MyGUI::newDelegate(this, &SettingsWindow::onButtonToggled);
        mFPSButton->eventMouseButtonClick += MyGUI::newDelegate(this, &SettingsWindow::onFpsToggled);
        mMenuTransparencySlider->eventScrollChangePosition += MyGUI::newDelegate(this, &SettingsWindow::onSliderChangePosition);
        mToolTipDelaySlider->eventScrollChangePosition += MyGUI::newDelegate(this, &SettingsWindow::onSliderChangePosition);
        mViewDistanceSlider->eventScrollChangePosition += MyGUI::newDelegate(this, &SettingsWindow::onSliderChangePosition);
        mResolutionList->eventListChangePosition += MyGUI::newDelegate(this, &SettingsWindow::onResolutionSelected);

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
        const Ogre::StringVector& videoModes = rs->getConfigOptions()["Video Mode"].possibleValues;
        for (Ogre::StringVector::const_iterator it=videoModes.begin();
            it!=videoModes.end(); ++it)
        {
            mResolutionList->addItem(*it);
        }

        // read settings
        int menu_transparency = (mMenuTransparencySlider->getScrollRange()-1) * Settings::Manager::getFloat("menu transparency", "GUI");
        mMenuTransparencySlider->setScrollPosition(menu_transparency);
        int tooltip_delay = (mToolTipDelaySlider->getScrollRange()-1) * Settings::Manager::getFloat("tooltip delay", "GUI");
        mToolTipDelaySlider->setScrollPosition(tooltip_delay);

        float val = (Settings::Manager::getFloat("max viewing distance", "Viewing distance")-2000)/(5600-2000);
        int viewdist = (mViewDistanceSlider->getScrollRange()-1) * val;
        mViewDistanceSlider->setScrollPosition(viewdist);

        mMasterVolumeSlider->setScrollPosition(Settings::Manager::getFloat("master volume", "Sound") * (mMasterVolumeSlider->getScrollRange()-1));
        mMusicVolumeSlider->setScrollPosition(Settings::Manager::getFloat("music volume", "Sound") * (mMusicVolumeSlider->getScrollRange()-1));
        mEffectsVolumeSlider->setScrollPosition(Settings::Manager::getFloat("sfx volume", "Sound") * (mEffectsVolumeSlider->getScrollRange()-1));
        mFootstepsVolumeSlider->setScrollPosition(Settings::Manager::getFloat("footsteps volume", "Sound") * (mFootstepsVolumeSlider->getScrollRange()-1));
        mVoiceVolumeSlider->setScrollPosition(Settings::Manager::getFloat("voice volume", "Sound") * (mVoiceVolumeSlider->getScrollRange()-1));

        mFullscreenButton->setCaptionWithReplacing(Settings::Manager::getBool("fullscreen", "Video") ? "#{sOn}" : "#{sOff}");
        mVSyncButton->setCaptionWithReplacing(Settings::Manager::getBool("vsync", "Video") ? "#{sOn}": "#{sOff}");
        mFPSButton->setCaptionWithReplacing(fpsLevelToStr(Settings::Manager::getInt("fps", "HUD")));
    }

    void SettingsWindow::onOkButtonClicked(MyGUI::Widget* _sender)
    {
        mWindowManager.popGuiMode();
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
        dialog->eventCancelClicked += MyGUI::newDelegate(this, &SettingsWindow::onResolutionAccept);
    }

    void SettingsWindow::onResolutionAccept()
    {
        std::string resStr = mResolutionList->getItemNameAt(mResolutionList->getIndexSelected());
        size_t xPos = resStr.find("x");
        std::string resXStr = resStr.substr(0, xPos-1);
        Ogre::StringUtil::trim(resXStr);
        std::string resYStr = resStr.substr(xPos+2, resStr.size()-(xPos+2));
        Ogre::StringUtil::trim(resYStr);
        int resX = boost::lexical_cast<int>(resXStr);
        int resY = boost::lexical_cast<int>(resYStr);

        Settings::Manager::setInt("resolution x", "Video", resX);
        Settings::Manager::setInt("resolution y", "Video", resY);

        apply();
        mResolutionList->setIndexSelected(MyGUI::ITEM_NONE);
    }

    void SettingsWindow::onResolutionCancel()
    {
        mResolutionList->setIndexSelected(MyGUI::ITEM_NONE);
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
                size_t xPos = resStr.find("x");
                std::string resXStr = resStr.substr(0, xPos-1);
                Ogre::StringUtil::trim(resXStr);
                std::string resYStr = resStr.substr(xPos+2, resStr.size()-(xPos+2));
                Ogre::StringUtil::trim(resYStr);
                int resX = boost::lexical_cast<int>(resXStr);
                int resY = boost::lexical_cast<int>(resYStr);

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
    }

    void SettingsWindow::onFpsToggled(MyGUI::Widget* _sender)
    {
        int newLevel = (Settings::Manager::getInt("fps", "HUD") + 1) % 3;
        Settings::Manager::setInt("fps", "HUD", newLevel);
        mFPSButton->setCaptionWithReplacing(fpsLevelToStr(newLevel));
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
            Settings::Manager::setFloat("max viewing distance", "Viewing distance", (1-val) * 2000 + val * 5600);
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

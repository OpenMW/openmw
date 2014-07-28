#include "settingswindow.hpp"

#include <OgreRoot.h>
#include <OgrePlugin.h>

#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/math/common_factor_rt.hpp>

#include <SDL_video.h>

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
        if (val == "anisotropic")
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

    const char* checkButtonType = "CheckButton";
    const char* sliderType = "Slider";

    std::string getSettingType(MyGUI::Widget* widget)
    {
        return widget->getUserString("SettingType");
    }

    std::string getSettingName(MyGUI::Widget* widget)
    {
        return widget->getUserString("SettingName");
    }

    std::string getSettingCategory(MyGUI::Widget* widget)
    {
        return widget->getUserString("SettingCategory");
    }

    std::string getSettingValueType(MyGUI::Widget* widget)
    {
        return widget->getUserString("SettingValueType");
    }

    void getSettingMinMax(MyGUI::Widget* widget, float& min, float& max)
    {
        const char* settingMin = "SettingMin";
        const char* settingMax = "SettingMax";
        min = 0.f;
        max = 1.f;
        if (!widget->getUserString(settingMin).empty())
            min = boost::lexical_cast<float>(widget->getUserString(settingMin));
        if (!widget->getUserString(settingMax).empty())
            max = boost::lexical_cast<float>(widget->getUserString(settingMax));
    }
}

namespace MWGui
{
    void SettingsWindow::configureWidgets(MyGUI::Widget* widget)
    {
        MyGUI::EnumeratorWidgetPtr widgets = widget->getEnumerator();
        while (widgets.next())
        {
            MyGUI::Widget* current = widgets.current();

            std::string type = getSettingType(current);
            if (type == checkButtonType)
            {
                std::string initialValue = Settings::Manager::getBool(getSettingName(current),
                                                                      getSettingCategory(current))
                        ? "#{sOn}" : "#{sOff}";
                current->castType<MyGUI::Button>()->setCaptionWithReplacing(initialValue);
                current->eventMouseButtonClick += MyGUI::newDelegate(this, &SettingsWindow::onButtonToggled);
            }
            if (type == sliderType)
            {
                MyGUI::ScrollBar* scroll = current->castType<MyGUI::ScrollBar>();
                if (getSettingValueType(current) == "Float")
                {
                    // TODO: ScrollBar isn't meant for this. should probably use a dedicated FloatSlider widget
                    float min,max;
                    getSettingMinMax(scroll, min, max);
                    float value = Settings::Manager::getFloat(getSettingName(current), getSettingCategory(current));
                    value = (value-min)/(max-min);

                    scroll->setScrollPosition( value * (scroll->getScrollRange()-1));
                }
                else
                {
                    int value = Settings::Manager::getFloat(getSettingName(current), getSettingCategory(current));
                    scroll->setScrollPosition(value);
                }
                scroll->eventScrollChangePosition += MyGUI::newDelegate(this, &SettingsWindow::onSliderChangePosition);
            }

            configureWidgets(current);
        }
    }

    SettingsWindow::SettingsWindow() :
        WindowBase("openmw_settings_window.layout")
    {
        configureWidgets(mMainWidget);

        setTitle("#{sOptions}");

        getWidget(mOkButton, "OkButton");
        getWidget(mResolutionList, "ResolutionList");
        getWidget(mFullscreenButton, "FullscreenButton");
        getWidget(mVSyncButton, "VSyncButton");
        getWidget(mFPSButton, "FPSButton");
        getWidget(mFOVSlider, "FOVSlider");
        getWidget(mAnisotropySlider, "AnisotropySlider");
        getWidget(mTextureFilteringButton, "TextureFilteringButton");
        getWidget(mAnisotropyLabel, "AnisotropyLabel");
        getWidget(mAnisotropyBox, "AnisotropyBox");
        getWidget(mShadersButton, "ShadersButton");
        getWidget(mShaderModeButton, "ShaderModeButton");
        getWidget(mShadowsEnabledButton, "ShadowsEnabledButton");
        getWidget(mShadowsTextureSize, "ShadowsTextureSize");
        getWidget(mControlsBox, "ControlsBox");
        getWidget(mResetControlsButton, "ResetControlsButton");
        getWidget(mRefractionButton, "RefractionButton");
        getWidget(mDifficultySlider, "DifficultySlider");

        mOkButton->eventMouseButtonClick += MyGUI::newDelegate(this, &SettingsWindow::onOkButtonClicked);
        mShaderModeButton->eventMouseButtonClick += MyGUI::newDelegate(this, &SettingsWindow::onShaderModeToggled);
        mTextureFilteringButton->eventComboChangePosition += MyGUI::newDelegate(this, &SettingsWindow::onTextureFilteringChanged);
        mFPSButton->eventMouseButtonClick += MyGUI::newDelegate(this, &SettingsWindow::onFpsToggled);
        mResolutionList->eventListChangePosition += MyGUI::newDelegate(this, &SettingsWindow::onResolutionSelected);

        mShadowsTextureSize->eventComboChangePosition += MyGUI::newDelegate(this, &SettingsWindow::onShadowTextureSizeChanged);

        center();

        mResetControlsButton->eventMouseButtonClick += MyGUI::newDelegate(this, &SettingsWindow::onResetDefaultBindings);

        // fill resolution list
        int screen = Settings::Manager::getInt("screen", "Video");
        int numDisplayModes = SDL_GetNumDisplayModes(screen);
        std::vector < std::pair<int, int> > resolutions;
        for (int i = 0; i < numDisplayModes; i++)
        {
            SDL_DisplayMode mode;
            SDL_GetDisplayMode(screen, i, &mode);
            resolutions.push_back(std::make_pair(mode.w, mode.h));
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

        std::string tf = Settings::Manager::getString("texture filtering", "General");
        mTextureFilteringButton->setCaption(textureFilteringToStr(tf));
        mAnisotropyLabel->setCaption("Anisotropy (" + boost::lexical_cast<std::string>(Settings::Manager::getInt("anisotropy", "General")) + ")");

        mShadowsTextureSize->setCaption (Settings::Manager::getString ("texture size", "Shadows"));

        mShaderModeButton->setCaption (Settings::Manager::getString("shader mode", "General"));

        if (!Settings::Manager::getBool("shaders", "Objects"))
        {
            mRefractionButton->setEnabled(false);
            mShadowsEnabledButton->setEnabled(false);
        }

        mFPSButton->setCaptionWithReplacing(fpsLevelToStr(Settings::Manager::getInt("fps", "HUD")));

        MyGUI::TextBox* fovText;
        getWidget(fovText, "FovText");
        fovText->setCaption("Field of View (" + boost::lexical_cast<std::string>(int(Settings::Manager::getInt("field of view", "General"))) + ")");

        MyGUI::TextBox* diffText;
        getWidget(diffText, "DifficultyText");
        diffText->setCaptionWithReplacing("#{sDifficulty} (" + boost::lexical_cast<std::string>(int(Settings::Manager::getInt("difficulty", "Game"))) + ")");
    }

    void SettingsWindow::onOkButtonClicked(MyGUI::Widget* _sender)
    {
        exit();
    }

    void SettingsWindow::onResolutionSelected(MyGUI::ListBox* _sender, size_t index)
    {
        if (index == MyGUI::ITEM_NONE)
            return;

        ConfirmationDialog* dialog = MWBase::Environment::get().getWindowManager()->getConfirmationDialog();
        dialog->open("#{sNotifyMessage67}");
        dialog->eventOkClicked.clear();
        dialog->eventOkClicked += MyGUI::newDelegate(this, &SettingsWindow::onResolutionAccept);
        dialog->eventCancelClicked.clear();
        dialog->eventCancelClicked += MyGUI::newDelegate(this, &SettingsWindow::onResolutionCancel);
    }

    void SettingsWindow::onResolutionAccept()
    {
        std::string resStr = mResolutionList->getItemNameAt(mResolutionList->getIndexSelected());
        int resX, resY;
        parseResolution (resX, resY, resStr);

        Settings::Manager::setInt("resolution x", "Video", resX);
        Settings::Manager::setInt("resolution y", "Video", resY);

        apply();
    }

    void SettingsWindow::onResolutionCancel()
    {
        mResolutionList->setIndexSelected(MyGUI::ITEM_NONE);
    }

    void SettingsWindow::onShadowTextureSizeChanged(MyGUI::ComboBox *_sender, size_t pos)
    {
        Settings::Manager::setString("texture size", "Shadows", _sender->getItemNameAt(pos));
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

        if (_sender == mVSyncButton)
        {
            // Ogre::Window::setVSyncEnabled is bugged in 1.8
#if OGRE_VERSION < (1 << 16 | 9 << 8 | 0)
            MWBase::Environment::get().getWindowManager()->
                messageBox("VSync will be applied after a restart", std::vector<std::string>());
#endif
        }

        if (_sender == mShadersButton)
        {
            if (newState == false)
            {
                // refraction needs shaders to display underwater fog
                mRefractionButton->setCaptionWithReplacing("#{sOff}");
                mRefractionButton->setEnabled(false);

                Settings::Manager::setBool("refraction", "Water", false);

                // shadows not supported
                mShadowsEnabledButton->setEnabled(false);
                mShadowsEnabledButton->setCaptionWithReplacing("#{sOff}");
                Settings::Manager::setBool("enabled", "Shadows", false);
            }
            else
            {
                // re-enable
                mRefractionButton->setEnabled(true);

                mShadowsEnabledButton->setEnabled(true);
            }
        }

        if (_sender == mFullscreenButton)
        {
            // check if this resolution is supported in fullscreen
            if (mResolutionList->getIndexSelected() != MyGUI::ITEM_NONE)
            {
                std::string resStr = mResolutionList->getItemNameAt(mResolutionList->getIndexSelected());
                int resX, resY;
                parseResolution (resX, resY, resStr);
                Settings::Manager::setInt("resolution x", "Video", resX);
                Settings::Manager::setInt("resolution y", "Video", resY);
            }

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
                return;
            }
        }

        if (getSettingType(_sender) == checkButtonType)
        {
            Settings::Manager::setBool(getSettingName(_sender), getSettingCategory(_sender), newState);
            apply();
            return;
        }
    }

    void SettingsWindow::onShaderModeToggled(MyGUI::Widget* _sender)
    {
        std::string val = static_cast<MyGUI::Button*>(_sender)->getCaption();
        val = hlslGlsl();

        static_cast<MyGUI::Button*>(_sender)->setCaption(val);

        Settings::Manager::setString("shader mode", "General", val);

        apply();
    }

    void SettingsWindow::onFpsToggled(MyGUI::Widget* _sender)
    {
        int newLevel = (Settings::Manager::getInt("fps", "HUD") + 1) % 3;
        Settings::Manager::setInt("fps", "HUD", newLevel);
        mFPSButton->setCaptionWithReplacing(fpsLevelToStr(newLevel));
        apply();
    }

    void SettingsWindow::onTextureFilteringChanged(MyGUI::ComboBox* _sender, size_t pos)
    {
        Settings::Manager::setString("texture filtering", "General", Misc::StringUtils::lowerCase(_sender->getItemNameAt(pos)));
        apply();
    }

    void SettingsWindow::onSliderChangePosition(MyGUI::ScrollBar* scroller, size_t pos)
    {
        if (getSettingType(scroller) == "Slider")
        {
            if (getSettingValueType(scroller) == "Float")
            {
                float value = pos / float(scroller->getScrollRange()-1);

                float min,max;
                getSettingMinMax(scroller, min, max);
                value = min + (max-min) * value;
                Settings::Manager::setFloat(getSettingName(scroller), getSettingCategory(scroller), value);

                if (scroller == mFOVSlider)
                {
                    MyGUI::TextBox* fovText;
                    getWidget(fovText, "FovText");
                    fovText->setCaption("Field of View (" + boost::lexical_cast<std::string>(int(value)) + ")");
                }
                if (scroller == mDifficultySlider)
                {
                    MyGUI::TextBox* diffText;
                    getWidget(diffText, "DifficultyText");
                    diffText->setCaptionWithReplacing("#{sDifficulty} (" + boost::lexical_cast<std::string>(int(value)) + ")");
                }
            }
            else
            {
                Settings::Manager::setInt(getSettingName(scroller), getSettingCategory(scroller), pos);
                if (scroller == mAnisotropySlider)
                {
                    mAnisotropyLabel->setCaption("Anisotropy (" + boost::lexical_cast<std::string>(pos) + ")");
                }
            }
            apply();
        }
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

        // Canvas size must be expressed with VScroll disabled, otherwise MyGUI would expand the scroll area when the scrollbar is hidden
        mControlsBox->setVisibleVScroll(false);
        mControlsBox->setCanvasSize (mControlsBox->getWidth(), std::max(curH, mControlsBox->getHeight()));
        mControlsBox->setVisibleVScroll(true);
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

    void SettingsWindow::exit()
    {
        MWBase::Environment::get().getWindowManager()->removeGuiMode(GM_Settings);
    }
}

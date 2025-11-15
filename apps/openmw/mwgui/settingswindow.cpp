#include "settingswindow.hpp"

#include <array>
#include <iomanip>

#include <unicode/locid.h>

#include <MyGUI_ComboBox.h>
#include <MyGUI_Gui.h>
#include <MyGUI_LanguageManager.h>
#include <MyGUI_ScrollBar.h>
#include <MyGUI_ScrollView.h>
#include <MyGUI_TabControl.h>
#include <MyGUI_UString.h>
#include <MyGUI_Window.h>

#include <SDL_video.h>

#include <components/debug/debuglog.hpp>
#include <components/files/configurationmanager.hpp>
#include <components/l10n/manager.hpp>
#include <components/lua_ui/scriptsettings.hpp>
#include <components/misc/constants.hpp>
#include <components/misc/display.hpp>
#include <components/misc/pathhelpers.hpp>
#include <components/misc/strings/algorithm.hpp>
#include <components/misc/strings/format.hpp>
#include <components/resource/resourcesystem.hpp>
#include <components/resource/scenemanager.hpp>
#include <components/sceneutil/lightmanager.hpp>
#include <components/settings/values.hpp>
#include <components/vfs/manager.hpp>
#include <components/vfs/recursivedirectoryiterator.hpp>
#include <components/widgets/sharedstatebutton.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/inputmanager.hpp"
#include "../mwbase/mechanicsmanager.hpp"
#include "../mwbase/soundmanager.hpp"
#include "../mwbase/windowmanager.hpp"
#include "../mwbase/world.hpp"
#include "../mwlua/luamanagerimp.hpp"

#include "confirmationdialog.hpp"

namespace
{
    std::string_view textureFilteringToStr(const std::string& mipFilter, const std::string& magFilter)
    {
        if (mipFilter == "none")
            return "#{OMWEngine:TextureFilteringDisabled}";

        if (magFilter == "linear")
        {
            if (mipFilter == "linear")
                return "#{OMWEngine:TextureFilteringTrilinear}";
            if (mipFilter == "nearest")
                return "#{OMWEngine:TextureFilteringBilinear}";
        }
        else if (magFilter == "nearest")
            return "#{OMWEngine:TextureFilteringNearest}";

        Log(Debug::Warning) << "Warning: Invalid texture filtering options: " << mipFilter << ", " << magFilter;
        return "#{OMWEngine:TextureFilteringOther}";
    }

    MyGUI::UString lightingMethodToStr(SceneUtil::LightingMethod method)
    {
        std::string_view result;
        switch (method)
        {
            case SceneUtil::LightingMethod::FFP:
                result = "#{OMWEngine:LightingMethodLegacy}";
                break;
            case SceneUtil::LightingMethod::PerObjectUniform:
                result = "#{OMWEngine:LightingMethodShadersCompatibility}";
                break;
            case SceneUtil::LightingMethod::SingleUBO:
            default:
                result = "#{OMWEngine:LightingMethodShaders}";
                break;
        }

        return MyGUI::LanguageManager::getInstance().replaceTags(MyGUI::UString(result));
    }

    bool sortResolutions(std::pair<int, int> left, std::pair<int, int> right)
    {
        if (left.first == right.first)
            return left.second > right.second;
        return left.first > right.first;
    }

    const std::string_view checkButtonType = "CheckButton";
    const std::string_view sliderType = "Slider";

    std::string_view getSettingType(MyGUI::Widget* widget)
    {
        return widget->getUserString("SettingType");
    }

    std::string_view getSettingName(MyGUI::Widget* widget)
    {
        return widget->getUserString("SettingName");
    }

    std::string_view getSettingCategory(MyGUI::Widget* widget)
    {
        return widget->getUserString("SettingCategory");
    }

    std::string_view getSettingValueType(MyGUI::Widget* widget)
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
            min = MyGUI::utility::parseFloat(widget->getUserString(settingMin));
        if (!widget->getUserString(settingMax).empty())
            max = MyGUI::utility::parseFloat(widget->getUserString(settingMax));
    }

    void updateMaxLightsComboBox(MyGUI::ComboBox* box)
    {
        constexpr int min = 8;
        constexpr int max = 64;
        constexpr int increment = 8;
        const int maxLights = Settings::shaders().mMaxLights;
        // show increments of 8 in dropdown
        if (maxLights >= min && maxLights <= max && !(maxLights % increment))
            box->setIndexSelected((maxLights / increment) - 1);
        else
            box->setIndexSelected(MyGUI::ITEM_NONE);
    }
}

namespace MWGui
{
    void SettingsWindow::configureWidgets(MyGUI::Widget* widget, bool init)
    {
        MyGUI::EnumeratorWidgetPtr widgets = widget->getEnumerator();
        while (widgets.next())
        {
            MyGUI::Widget* current = widgets.current();

            std::string_view type = getSettingType(current);
            if (type == checkButtonType)
            {
                std::string_view initialValue
                    = Settings::get<bool>(getSettingCategory(current), getSettingName(current)) ? "#{Interface:On}"
                                                                                                : "#{Interface:Off}";
                current->castType<MyGUI::Button>()->setCaptionWithReplacing(initialValue);
                if (init)
                    current->eventMouseButtonClick += MyGUI::newDelegate(this, &SettingsWindow::onButtonToggled);
            }
            if (type == sliderType)
            {
                MyGUI::ScrollBar* scroll = current->castType<MyGUI::ScrollBar>();
                std::string valueStr;
                std::string_view valueType = getSettingValueType(current);
                if (valueType == "Float" || valueType == "Integer" || valueType == "Cell")
                {
                    // TODO: ScrollBar isn't meant for this. should probably use a dedicated FloatSlider widget
                    float min, max;
                    getSettingMinMax(scroll, min, max);
                    float value;

                    if (valueType == "Cell")
                    {
                        value = Settings::get<float>(getSettingCategory(current), getSettingName(current));
                        std::stringstream ss;
                        ss << std::fixed << std::setprecision(2) << value / Constants::CellSizeInUnits;
                        valueStr = ss.str();
                    }
                    else if (valueType == "Float")
                    {
                        value = Settings::get<float>(getSettingCategory(current), getSettingName(current));
                        std::stringstream ss;
                        ss << std::fixed << std::setprecision(2) << value;
                        valueStr = ss.str();
                    }
                    else
                    {
                        const int intValue = Settings::get<int>(getSettingCategory(current), getSettingName(current));
                        valueStr = MyGUI::utility::toString(intValue);
                        value = static_cast<float>(intValue);
                    }

                    value = std::clamp(value, min, max);
                    value = (value - min) / (max - min);

                    scroll->setScrollPosition(static_cast<size_t>(value * (scroll->getScrollRange() - 1)));
                }
                else
                {
                    const int value = Settings::get<int>(getSettingCategory(current), getSettingName(current));
                    valueStr = MyGUI::utility::toString(value);
                    scroll->setScrollPosition(value);
                }
                if (init)
                    scroll->eventScrollChangePosition
                        += MyGUI::newDelegate(this, &SettingsWindow::onSliderChangePosition);
                if (scroll->getVisible())
                    updateSliderLabel(scroll, valueStr);
            }

            configureWidgets(current, init);
        }
    }

    void SettingsWindow::onFrame(float duration)
    {
        if (mScriptView->getVisible())
        {
            const auto scriptsSize = mScriptAdapter->getSize();
            if (mScriptView->getCanvasSize() != scriptsSize)
                mScriptView->setCanvasSize(scriptsSize);
        }
    }

    void SettingsWindow::updateSliderLabel(MyGUI::ScrollBar* scroller, const std::string& value)
    {
        auto labelWidgetName = scroller->getUserString("SettingLabelWidget");
        if (!labelWidgetName.empty())
        {
            MyGUI::TextBox* textBox;
            getWidget(textBox, labelWidgetName);
            std::string labelCaption{ scroller->getUserString("SettingLabelCaption") };
            labelCaption = Misc::StringUtils::format(labelCaption, value);
            textBox->setCaptionWithReplacing(labelCaption);
        }
    }

    SettingsWindow::SettingsWindow(Files::ConfigurationManager& cfgMgr)
        : WindowBase("openmw_settings_window.layout")
        , mKeyboardMode(true)
        , mCurrentPage(static_cast<size_t>(-1))
        , mCfgMgr(cfgMgr)
    {
        const bool terrain = Settings::terrain().mDistantTerrain;
        const std::string_view widgetName = terrain ? "RenderingDistanceSlider" : "LargeRenderingDistanceSlider";
        MyGUI::Widget* unusedSlider;
        getWidget(unusedSlider, widgetName);
        unusedSlider->setVisible(false);

        configureWidgets(mMainWidget, true);

        setTitle("#{OMWEngine:SettingsWindow}");

        getWidget(mSettingsTab, "SettingsTab");
        getWidget(mOkButton, "OkButton");
        getWidget(mResolutionList, "ResolutionList");
        getWidget(mWindowModeList, "WindowModeList");
        getWidget(mVSyncModeList, "VSyncModeList");
        getWidget(mWindowBorderButton, "WindowBorderButton");
        getWidget(mTextureFilteringButton, "TextureFilteringButton");
        getWidget(mControlsBox, "ControlsBox");
        getWidget(mResetControlsButton, "ResetControlsButton");
        getWidget(mKeyboardSwitch, "KeyboardButton");
        getWidget(mControllerSwitch, "ControllerButton");
        getWidget(mWaterRefractionButton, "WaterRefractionButton");
        getWidget(mSunlightScatteringButton, "SunlightScatteringButton");
        getWidget(mWobblyShoresButton, "WobblyShoresButton");
        getWidget(mWaterTextureSize, "WaterTextureSize");
        getWidget(mWaterReflectionDetail, "WaterReflectionDetail");
        getWidget(mWaterRainRippleDetail, "WaterRainRippleDetail");
        getWidget(mPrimaryLanguage, "PrimaryLanguage");
        getWidget(mSecondaryLanguage, "SecondaryLanguage");
        getWidget(mGmstOverridesL10n, "GmstOverridesL10nButton");
        getWidget(mWindowModeHint, "WindowModeHint");
        getWidget(mLightingMethodButton, "LightingMethodButton");
        getWidget(mLightsResetButton, "LightsResetButton");
        getWidget(mMaxLights, "MaxLights");
        getWidget(mScriptFilter, "ScriptFilter");
        getWidget(mScriptList, "ScriptList");
        getWidget(mScriptBox, "ScriptBox");
        getWidget(mScriptView, "ScriptView");
        getWidget(mScriptAdapter, "ScriptAdapter");
        getWidget(mScriptDisabled, "ScriptDisabled");

#ifndef WIN32
        // hide gamma controls since it currently does not work under Linux
        MyGUI::ScrollBar* gammaSlider;
        getWidget(gammaSlider, "GammaSlider");
        gammaSlider->setVisible(false);
        MyGUI::TextBox* textBox;
        getWidget(textBox, "GammaText");
        textBox->setVisible(false);
        getWidget(textBox, "GammaTextDark");
        textBox->setVisible(false);
        getWidget(textBox, "GammaTextLight");
        textBox->setVisible(false);
#endif

        mMainWidget->castType<MyGUI::Window>()->eventWindowChangeCoord
            += MyGUI::newDelegate(this, &SettingsWindow::onWindowResize);

        mSettingsTab->eventTabChangeSelect += MyGUI::newDelegate(this, &SettingsWindow::onTabChanged);
        mOkButton->eventMouseButtonClick += MyGUI::newDelegate(this, &SettingsWindow::onOkButtonClicked);
        mTextureFilteringButton->eventComboChangePosition
            += MyGUI::newDelegate(this, &SettingsWindow::onTextureFilteringChanged);
        mResolutionList->eventListChangePosition += MyGUI::newDelegate(this, &SettingsWindow::onResolutionSelected);

        mWaterRefractionButton->eventMouseButtonClick
            += MyGUI::newDelegate(this, &SettingsWindow::onRefractionButtonClicked);
        mWaterTextureSize->eventComboChangePosition
            += MyGUI::newDelegate(this, &SettingsWindow::onWaterTextureSizeChanged);
        mWaterReflectionDetail->eventComboChangePosition
            += MyGUI::newDelegate(this, &SettingsWindow::onWaterReflectionDetailChanged);
        mWaterRainRippleDetail->eventComboChangePosition
            += MyGUI::newDelegate(this, &SettingsWindow::onWaterRainRippleDetailChanged);

        mLightingMethodButton->eventComboChangePosition
            += MyGUI::newDelegate(this, &SettingsWindow::onLightingMethodButtonChanged);
        mLightsResetButton->eventMouseButtonClick
            += MyGUI::newDelegate(this, &SettingsWindow::onLightsResetButtonClicked);
        mMaxLights->eventComboChangePosition += MyGUI::newDelegate(this, &SettingsWindow::onMaxLightsChanged);

        mWindowModeList->eventComboChangePosition += MyGUI::newDelegate(this, &SettingsWindow::onWindowModeChanged);
        mVSyncModeList->eventComboChangePosition += MyGUI::newDelegate(this, &SettingsWindow::onVSyncModeChanged);

        mKeyboardSwitch->eventMouseButtonClick += MyGUI::newDelegate(this, &SettingsWindow::onKeyboardSwitchClicked);
        mControllerSwitch->eventMouseButtonClick
            += MyGUI::newDelegate(this, &SettingsWindow::onControllerSwitchClicked);

        mPrimaryLanguage->eventComboChangePosition
            += MyGUI::newDelegate(this, &SettingsWindow::onPrimaryLanguageChanged);
        mSecondaryLanguage->eventComboChangePosition
            += MyGUI::newDelegate(this, &SettingsWindow::onSecondaryLanguageChanged);
        mGmstOverridesL10n->eventMouseButtonClick
            += MyGUI::newDelegate(this, &SettingsWindow::onGmstOverridesL10nChanged);

        computeMinimumWindowSize();

        center();

        mResetControlsButton->eventMouseButtonClick
            += MyGUI::newDelegate(this, &SettingsWindow::onResetDefaultBindings);

        // fill resolution list
        const int screen = Settings::video().mScreen;
        int numDisplayModes = SDL_GetNumDisplayModes(screen);
        std::vector<std::pair<int, int>> resolutions;
        for (int i = 0; i < numDisplayModes; i++)
        {
            SDL_DisplayMode mode;
            SDL_GetDisplayMode(screen, i, &mode);
            resolutions.emplace_back(mode.w, mode.h);
        }
        std::sort(resolutions.begin(), resolutions.end(), sortResolutions);
        for (std::pair<int, int>& resolution : resolutions)
        {
            std::string str = Misc::getResolutionText(resolution.first, resolution.second);

            if (mResolutionList->findItemIndexWith(str) == MyGUI::ITEM_NONE)
                mResolutionList->addItem(str, resolution);
        }
        highlightCurrentResolution();

        mTextureFilteringButton->setCaptionWithReplacing(
            textureFilteringToStr(Settings::general().mTextureMipmap, Settings::general().mTextureMinFilter));

        int waterTextureSize = Settings::water().mRttSize;
        if (waterTextureSize >= 512)
            mWaterTextureSize->setIndexSelected(0);
        if (waterTextureSize >= 1024)
            mWaterTextureSize->setIndexSelected(1);
        if (waterTextureSize >= 2048)
            mWaterTextureSize->setIndexSelected(2);

        const int waterReflectionDetail = Settings::water().mReflectionDetail;
        mWaterReflectionDetail->setIndexSelected(waterReflectionDetail);

        const int waterRainRippleDetail = Settings::water().mRainRippleDetail;
        mWaterRainRippleDetail->setIndexSelected(waterRainRippleDetail);

        const bool waterRefraction = Settings::water().mRefraction;
        mSunlightScatteringButton->setEnabled(waterRefraction);
        mWobblyShoresButton->setEnabled(waterRefraction);

        updateMaxLightsComboBox(mMaxLights);

        const Settings::WindowMode windowMode = Settings::video().mWindowMode;
        mWindowBorderButton->setEnabled(
            windowMode != Settings::WindowMode::Fullscreen && windowMode != Settings::WindowMode::WindowedFullscreen);

        mWindowModeHint->setVisible(windowMode == Settings::WindowMode::WindowedFullscreen);

        mKeyboardSwitch->setStateSelected(true);
        mControllerSwitch->setStateSelected(false);

        mScriptFilter->eventEditTextChange += MyGUI::newDelegate(this, &SettingsWindow::onScriptFilterChange);
        mScriptList->eventListMouseItemActivate += MyGUI::newDelegate(this, &SettingsWindow::onScriptListSelection);

        std::vector<std::string> availableLanguages;
        const VFS::Manager* vfs = MWBase::Environment::get().getResourceSystem()->getVFS();
        constexpr VFS::Path::NormalizedView l10n("l10n/");
        for (const auto& path : vfs->getRecursiveDirectoryIterator(l10n))
        {
            if (Misc::getFileExtension(path) == "yaml")
            {
                std::string localeName(Misc::stemFile(path));
                if (localeName == "gmst")
                    continue; // fake locale to get gmst strings from content files
                if (std::find(availableLanguages.begin(), availableLanguages.end(), localeName)
                    == availableLanguages.end())
                    availableLanguages.push_back(std::move(localeName));
            }
        }

        std::sort(availableLanguages.begin(), availableLanguages.end());

        std::vector<std::string> currentLocales = Settings::general().mPreferredLocales;
        if (currentLocales.empty())
            currentLocales.push_back("en");

        icu::Locale primaryLocale(currentLocales[0].c_str());

        mPrimaryLanguage->removeAllItems();
        mPrimaryLanguage->setIndexSelected(MyGUI::ITEM_NONE);

        mSecondaryLanguage->removeAllItems();
        mSecondaryLanguage->addItem(
            MyGUI::LanguageManager::getInstance().replaceTags("#{Interface:None}"), std::string());
        mSecondaryLanguage->setIndexSelected(0);

        size_t i = 0;
        for (const auto& language : availableLanguages)
        {
            icu::Locale locale(language.c_str());

            icu::UnicodeString str(language.c_str());
            locale.getDisplayName(primaryLocale, str);
            std::string localeString;
            str.toUTF8String(localeString);

            mPrimaryLanguage->addItem(localeString, language);
            mSecondaryLanguage->addItem(localeString, language);

            if (language == currentLocales[0])
                mPrimaryLanguage->setIndexSelected(i);
            if (currentLocales.size() > 1 && language == currentLocales[1])
                mSecondaryLanguage->setIndexSelected(i + 1);

            i++;
        }

        mControllerButtons.mA = "#{Interface:Select}";
        mControllerButtons.mB = "#{Interface:OK}";
        mControllerButtons.mLStick = "#{Interface:Mouse}";
    }

    void SettingsWindow::onTabChanged(MyGUI::TabControl* /*sender*/, size_t /*index*/)
    {
        resetScrollbars();
    }

    void SettingsWindow::onOkButtonClicked(MyGUI::Widget* /*sender*/)
    {
        MWBase::Environment::get().getWindowManager()->toggleSettingsWindow();
    }

    void SettingsWindow::onResolutionSelected(MyGUI::ListBox* /*sender*/, size_t index)
    {
        if (index == MyGUI::ITEM_NONE)
            return;

        ConfirmationDialog* dialog = MWBase::Environment::get().getWindowManager()->getConfirmationDialog();
        dialog->askForConfirmation("#{OMWEngine:ConfirmResolution}");
        dialog->eventOkClicked.clear();
        dialog->eventOkClicked += MyGUI::newDelegate(this, &SettingsWindow::onResolutionAccept);
        dialog->eventCancelClicked.clear();
        dialog->eventCancelClicked += MyGUI::newDelegate(this, &SettingsWindow::onResolutionCancel);
    }

    void SettingsWindow::onResolutionAccept()
    {
        auto resolution = mResolutionList->getItemDataAt<std::pair<int, int>>(mResolutionList->getIndexSelected());
        if (resolution)
        {
            Settings::video().mResolutionX.set(resolution->first);
            Settings::video().mResolutionY.set(resolution->second);

            apply();
        }
    }

    void SettingsWindow::onResolutionCancel()
    {
        highlightCurrentResolution();
    }

    void SettingsWindow::highlightCurrentResolution()
    {
        mResolutionList->setIndexSelected(MyGUI::ITEM_NONE);

        const int currentX = Settings::video().mResolutionX;
        const int currentY = Settings::video().mResolutionY;

        for (size_t i = 0; i < mResolutionList->getItemCount(); ++i)
        {
            auto resolution = mResolutionList->getItemDataAt<std::pair<int, int>>(i);
            if (resolution && resolution->first == currentX && resolution->second == currentY)
            {
                mResolutionList->setIndexSelected(i);
                break;
            }
        }
    }

    void SettingsWindow::onRefractionButtonClicked(MyGUI::Widget* /*sender*/)
    {
        const bool refractionEnabled = Settings::water().mRefraction;

        mSunlightScatteringButton->setEnabled(refractionEnabled);
        mWobblyShoresButton->setEnabled(refractionEnabled);
    }

    void SettingsWindow::onWaterTextureSizeChanged(MyGUI::ComboBox* /*sender*/, size_t pos)
    {
        int size = 0;
        if (pos == 0)
            size = 512;
        else if (pos == 1)
            size = 1024;
        else if (pos == 2)
            size = 2048;
        Settings::water().mRttSize.set(size);
        apply();
    }

    void SettingsWindow::onWaterReflectionDetailChanged(MyGUI::ComboBox* /*sender*/, size_t pos)
    {
        Settings::water().mReflectionDetail.set(static_cast<int>(pos));
        apply();
    }

    void SettingsWindow::onWaterRainRippleDetailChanged(MyGUI::ComboBox* /*sender*/, size_t pos)
    {
        Settings::water().mRainRippleDetail.set(static_cast<int>(pos));
        apply();
    }

    void SettingsWindow::onLightingMethodButtonChanged(MyGUI::ComboBox* sender, size_t pos)
    {
        if (pos == MyGUI::ITEM_NONE)
            return;

        sender->setCaptionWithReplacing(sender->getItemNameAt(sender->getIndexSelected()));

        MWBase::Environment::get().getWindowManager()->interactiveMessageBox(
            "#{OMWEngine:ChangeRequiresRestart}", { "#{Interface:OK}" }, true);

        Settings::shaders().mLightingMethod.set(
            Settings::parseLightingMethod(*sender->getItemDataAt<std::string>(pos)));
        apply();
    }

    void SettingsWindow::onLanguageChanged(size_t langPriority, MyGUI::ComboBox* sender, size_t pos)
    {
        if (pos == MyGUI::ITEM_NONE)
            return;

        sender->setCaptionWithReplacing(sender->getItemNameAt(sender->getIndexSelected()));

        MWBase::Environment::get().getWindowManager()->interactiveMessageBox(
            "#{OMWEngine:ChangeRequiresRestart}", { "#{Interface:OK}" }, true);

        std::vector<std::string> currentLocales = Settings::general().mPreferredLocales;
        if (currentLocales.size() <= langPriority)
            currentLocales.resize(langPriority + 1, "en");

        const auto& languageCode = *sender->getItemDataAt<std::string>(pos);
        if (!languageCode.empty())
            currentLocales[langPriority] = languageCode;
        else
            currentLocales.resize(1);

        Settings::general().mPreferredLocales.set(currentLocales);
    }

    void SettingsWindow::onGmstOverridesL10nChanged(MyGUI::Widget*)
    {
        MWBase::Environment::get().getWindowManager()->interactiveMessageBox(
            "#{OMWEngine:ChangeRequiresRestart}", { "#{Interface:OK}" }, true);
    }

    void SettingsWindow::onVSyncModeChanged(MyGUI::ComboBox* sender, size_t pos)
    {
        if (pos == MyGUI::ITEM_NONE)
            return;

        Settings::video().mVsyncMode.set(static_cast<SDLUtil::VSyncMode>(sender->getIndexSelected()));
        apply();
    }

    void SettingsWindow::onWindowModeChanged(MyGUI::ComboBox* sender, size_t pos)
    {
        if (pos == MyGUI::ITEM_NONE)
            return;

        const Settings::WindowMode windowMode = static_cast<Settings::WindowMode>(sender->getIndexSelected());
        if (windowMode == Settings::WindowMode::WindowedFullscreen)
        {
            mResolutionList->setEnabled(false);
            mWindowModeHint->setVisible(true);
        }
        else
        {
            mResolutionList->setEnabled(true);
            mWindowModeHint->setVisible(false);
        }

        if (windowMode == Settings::WindowMode::Windowed)
            mWindowBorderButton->setEnabled(true);
        else
            mWindowBorderButton->setEnabled(false);

        Settings::video().mWindowMode.set(windowMode);
        apply();
    }

    void SettingsWindow::onMaxLightsChanged(MyGUI::ComboBox* /*sender*/, size_t pos)
    {
        Settings::shaders().mMaxLights.set(8 * static_cast<int>(pos + 1));
        apply();
        configureWidgets(mMainWidget, false);
    }

    void SettingsWindow::onLightsResetButtonClicked(MyGUI::Widget* /*sender*/)
    {
        std::vector<std::string> buttons = { "#{Interface:Yes}", "#{Interface:No}" };
        MWBase::Environment::get().getWindowManager()->interactiveMessageBox(
            "#{OMWEngine:LightingResetToDefaults}", buttons, true);
        int selectedButton = MWBase::Environment::get().getWindowManager()->readPressedButton();
        if (selectedButton == 1 || selectedButton == -1)
            return;

        Settings::shaders().mForcePerPixelLighting.reset();
        Settings::shaders().mClassicFalloff.reset();
        Settings::shaders().mMatchSunlightToSun.reset();
        Settings::shaders().mLightBoundsMultiplier.reset();
        Settings::shaders().mMaximumLightDistance.reset();
        Settings::shaders().mLightFadeStart.reset();
        Settings::shaders().mMinimumInteriorBrightness.reset();
        Settings::shaders().mMaxLights.reset();
        Settings::shaders().mLightingMethod.reset();

        const SceneUtil::LightingMethod lightingMethod = Settings::shaders().mLightingMethod;
        const std::size_t lightIndex = mLightingMethodButton->findItemIndexWith(lightingMethodToStr(lightingMethod));
        mLightingMethodButton->setIndexSelected(lightIndex);
        updateMaxLightsComboBox(mMaxLights);

        apply();
        configureWidgets(mMainWidget, false);
    }

    void SettingsWindow::onButtonToggled(MyGUI::Widget* sender)
    {
        const std::string on = MWBase::Environment::get().getL10nManager()->getMessage("Interface", "On");
        const std::string off = MWBase::Environment::get().getL10nManager()->getMessage("Interface", "Off");
        bool newState;
        if (sender->castType<MyGUI::Button>()->getCaption() == on)
        {
            sender->castType<MyGUI::Button>()->setCaption(MyGUI::UString(off));
            newState = false;
        }
        else
        {
            sender->castType<MyGUI::Button>()->setCaption(MyGUI::UString(on));
            newState = true;
        }

        if (getSettingType(sender) == checkButtonType)
        {
            Settings::get<bool>(getSettingCategory(sender), getSettingName(sender)).set(newState);
            apply();
            return;
        }
    }

    void SettingsWindow::onTextureFilteringChanged(MyGUI::ComboBox* /*sender*/, size_t pos)
    {
        auto& generalSettings = Settings::general();
        switch (pos)
        {
            case 0: // Bilinear with mips
                generalSettings.mTextureMipmap.set("nearest");
                generalSettings.mTextureMagFilter.set("linear");
                generalSettings.mTextureMinFilter.set("linear");
                break;
            case 1: // Trilinear with mips
                generalSettings.mTextureMipmap.set("linear");
                generalSettings.mTextureMagFilter.set("linear");
                generalSettings.mTextureMinFilter.set("linear");
                break;
            default:
                Log(Debug::Warning) << "Unexpected texture filtering option pos " << pos;
                break;
        }

        apply();
    }

    void SettingsWindow::onResChange(int /*width*/, int /*height*/)
    {
        center();
        highlightCurrentResolution();
    }

    void SettingsWindow::onSliderChangePosition(MyGUI::ScrollBar* scroller, size_t pos)
    {
        if (getSettingType(scroller) == "Slider")
        {
            std::string valueStr;
            std::string_view valueType = getSettingValueType(scroller);
            if (valueType == "Float" || valueType == "Integer" || valueType == "Cell")
            {
                float value = pos / float(scroller->getScrollRange() - 1);

                float min, max;
                getSettingMinMax(scroller, min, max);
                value = min + (max - min) * value;

                if (valueType == "Cell")
                {
                    Settings::get<float>(getSettingCategory(scroller), getSettingName(scroller)).set(value);
                    std::stringstream ss;
                    ss << std::fixed << std::setprecision(2) << value / Constants::CellSizeInUnits;
                    valueStr = ss.str();
                }
                else if (valueType == "Float")
                {
                    Settings::get<float>(getSettingCategory(scroller), getSettingName(scroller)).set(value);
                    std::stringstream ss;
                    ss << std::fixed << std::setprecision(2) << value;
                    valueStr = ss.str();
                }
                else
                {
                    Settings::get<int>(getSettingCategory(scroller), getSettingName(scroller))
                        .set(static_cast<int>(value));
                    valueStr = MyGUI::utility::toString(int(value));
                }
            }
            else
            {
                Settings::get<int>(getSettingCategory(scroller), getSettingName(scroller)).set(static_cast<int>(pos));
                valueStr = MyGUI::utility::toString(pos);
            }
            updateSliderLabel(scroller, valueStr);

            apply();
        }
    }

    void SettingsWindow::apply()
    {
        const Settings::CategorySettingVector changed = Settings::Manager::getPendingChanges();
        MWBase::Environment::get().getWorld()->processChangedSettings(changed);
        MWBase::Environment::get().getSoundManager()->processChangedSettings(changed);
        MWBase::Environment::get().getWindowManager()->processChangedSettings(changed);
        MWBase::Environment::get().getInputManager()->processChangedSettings(changed);
        MWBase::Environment::get().getMechanicsManager()->processChangedSettings(changed);
        Settings::Manager::resetPendingChanges();
    }

    void SettingsWindow::onKeyboardSwitchClicked(MyGUI::Widget* /*sender*/)
    {
        if (mKeyboardMode)
            return;
        mKeyboardMode = true;
        mKeyboardSwitch->setStateSelected(true);
        mControllerSwitch->setStateSelected(false);
        updateControlsBox();
        resetScrollbars();
    }

    void SettingsWindow::onControllerSwitchClicked(MyGUI::Widget* /*sender*/)
    {
        if (!mKeyboardMode)
            return;
        mKeyboardMode = false;
        mKeyboardSwitch->setStateSelected(false);
        mControllerSwitch->setStateSelected(true);
        updateControlsBox();
        resetScrollbars();
    }

    void SettingsWindow::updateControlsBox()
    {
        while (mControlsBox->getChildCount())
            MyGUI::Gui::getInstance().destroyWidget(mControlsBox->getChildAt(0));

        MWBase::Environment::get().getWindowManager()->removeStaticMessageBox();
        const auto inputManager = MWBase::Environment::get().getInputManager();
        const auto& actions
            = mKeyboardMode ? inputManager->getActionKeySorting() : inputManager->getActionControllerSorting();

        for (const int& action : actions)
        {
            std::string desc{ inputManager->getActionDescription(action) };
            if (desc.empty())
                continue;

            std::string binding;
            if (mKeyboardMode)
                binding = inputManager->getActionKeyBindingName(action);
            else
                binding = inputManager->getActionControllerBindingName(action);

            Gui::SharedStateButton* leftText = mControlsBox->createWidget<Gui::SharedStateButton>(
                "SandTextButton", MyGUI::IntCoord(), MyGUI::Align::Default);
            leftText->setCaptionWithReplacing(desc);

            Gui::SharedStateButton* rightText = mControlsBox->createWidget<Gui::SharedStateButton>(
                "SandTextButton", MyGUI::IntCoord(), MyGUI::Align::Default);
            rightText->setCaptionWithReplacing(binding);
            rightText->setTextAlign(MyGUI::Align::Right);
            rightText->setUserData(action); // save the action id for callbacks
            rightText->eventMouseButtonClick += MyGUI::newDelegate(this, &SettingsWindow::onRebindAction);
            rightText->eventMouseWheel += MyGUI::newDelegate(this, &SettingsWindow::onInputTabMouseWheel);

            Gui::ButtonGroup group;
            group.push_back(leftText);
            group.push_back(rightText);
            Gui::SharedStateButton::createButtonGroup(group);
        }

        layoutControlsBox();
    }

    void SettingsWindow::updateLightSettings()
    {
        auto lightingMethod = MWBase::Environment::get().getResourceSystem()->getSceneManager()->getLightingMethod();
        MyGUI::UString lightingMethodStr = lightingMethodToStr(lightingMethod);

        mLightingMethodButton->removeAllItems();

        std::array<SceneUtil::LightingMethod, 3> methods = {
            SceneUtil::LightingMethod::FFP,
            SceneUtil::LightingMethod::PerObjectUniform,
            SceneUtil::LightingMethod::SingleUBO,
        };

        for (const auto& method : methods)
        {
            if (!MWBase::Environment::get().getResourceSystem()->getSceneManager()->isSupportedLightingMethod(method))
                continue;

            mLightingMethodButton->addItem(
                lightingMethodToStr(method), SceneUtil::LightManager::getLightingMethodString(method));
        }
        mLightingMethodButton->setIndexSelected(mLightingMethodButton->findItemIndexWith(lightingMethodStr));
    }

    void SettingsWindow::updateWindowModeSettings()
    {
        const Settings::WindowMode windowMode = Settings::video().mWindowMode;
        const std::size_t windowModeIndex = static_cast<std::size_t>(windowMode);

        mWindowModeList->setIndexSelected(windowModeIndex);

        if (windowMode != Settings::WindowMode::Windowed && windowModeIndex != MyGUI::ITEM_NONE)
        {
            // check if this resolution is supported in fullscreen
            if (mResolutionList->getIndexSelected() != MyGUI::ITEM_NONE)
            {
                auto resolution
                    = mResolutionList->getItemDataAt<std::pair<int, int>>(mResolutionList->getIndexSelected());
                if (resolution)
                {
                    Settings::video().mResolutionX.set(resolution->first);
                    Settings::video().mResolutionY.set(resolution->second);
                }
            }

            bool supported = false;
            int fallbackX = 0, fallbackY = 0;
            for (size_t i = 0; i < mResolutionList->getItemCount(); ++i)
            {
                auto resolution = mResolutionList->getItemDataAt<std::pair<int, int>>(i);
                if (!resolution)
                    continue;

                if (i == 0)
                {
                    fallbackX = resolution->first;
                    fallbackY = resolution->second;
                }

                if (resolution->first == Settings::video().mResolutionX
                    && resolution->second == Settings::video().mResolutionY)
                    supported = true;
            }

            if (!supported && mResolutionList->getItemCount())
            {
                if (fallbackX != 0 && fallbackY != 0)
                {
                    Settings::video().mResolutionX.set(fallbackX);
                    Settings::video().mResolutionY.set(fallbackY);
                }
            }

            mWindowBorderButton->setEnabled(false);
        }

        if (windowMode == Settings::WindowMode::WindowedFullscreen)
            mResolutionList->setEnabled(false);
    }

    void SettingsWindow::updateVSyncModeSettings()
    {
        mVSyncModeList->setIndexSelected(static_cast<size_t>(Settings::video().mVsyncMode));
    }

    void SettingsWindow::layoutControlsBox()
    {
        const int h = Settings::gui().mFontSize + 2;
        const int w = mControlsBox->getWidth() - 28;
        const int noWidgetsInRow = 2;
        const int totalH = static_cast<int>(mControlsBox->getChildCount() / noWidgetsInRow) * h;

        for (size_t i = 0; i < mControlsBox->getChildCount(); i++)
        {
            MyGUI::Widget* widget = mControlsBox->getChildAt(i);
            widget->setCoord(0, static_cast<int>(i / noWidgetsInRow * h), w, h);
        }

        // Canvas size must be expressed with VScroll disabled, otherwise MyGUI would expand the scroll area when the
        // scrollbar is hidden
        mControlsBox->setVisibleVScroll(false);
        mControlsBox->setCanvasSize(mControlsBox->getWidth(), std::max(totalH, mControlsBox->getHeight()));
        mControlsBox->setVisibleVScroll(true);
    }

    namespace
    {
        std::vector<std::string> splitString(const std::string& inputString)
        {
            std::istringstream stringStream(inputString);
            return { std::istream_iterator<std::string>(stringStream), std::istream_iterator<std::string>() };
        }
        double weightedSearch(std::string corpus, std::string patternString)
        {
            if (patternString.empty() || patternString.find_first_not_of(" ") == std::string::npos)
                return 1.0;

            Misc::StringUtils::lowerCaseInPlace(corpus);
            Misc::StringUtils::lowerCaseInPlace(patternString);

            const std::vector<std::string> patternArray = splitString(patternString);

            double numberOfMatches = 0.0;
            for (const std::string& word : patternArray)
                numberOfMatches += corpus.find(word) != std::string::npos ? 1.0 : 0.0;

            return numberOfMatches;
        }
    }

    void SettingsWindow::renderScriptSettings()
    {
        mScriptAdapter->detach();

        mScriptList->removeAllItems();
        mScriptView->setCanvasSize({ 0, 0 });

        struct WeightedPage
        {
            size_t mIndex;
            std::string mName;
            double mNameWeight;
            double mHintWeight;

            constexpr auto tie() const { return std::tie(mNameWeight, mHintWeight, mName); }

            constexpr bool operator<(const WeightedPage& rhs) const { return tie() < rhs.tie(); }
        };

        std::string searchPattern = mScriptFilter->getCaption();
        std::vector<WeightedPage> weightedPages;
        weightedPages.reserve(LuaUi::scriptSettingsPageCount());
        for (size_t i = 0; i < LuaUi::scriptSettingsPageCount(); ++i)
        {
            LuaUi::ScriptSettingsPage page = LuaUi::scriptSettingsPageAt(i);
            double nameWeight = weightedSearch(page.mName, searchPattern);
            double hintWeight = weightedSearch(page.mSearchHints, searchPattern);
            if ((nameWeight + hintWeight) > 0)
                weightedPages.push_back({ i, page.mName, -nameWeight, -hintWeight });
        }
        std::sort(weightedPages.begin(), weightedPages.end());
        for (const WeightedPage& weightedPage : weightedPages)
            mScriptList->addItem(weightedPage.mName, weightedPage.mIndex);

        // Hide script settings when the game world isn't loaded
        bool disabled = LuaUi::scriptSettingsPageCount() == 0;
        mScriptFilter->setVisible(!disabled);
        mScriptList->setVisible(!disabled);
        mScriptBox->setVisible(!disabled);
        mScriptDisabled->setVisible(disabled);

        LuaUi::attachPageAt(mCurrentPage, mScriptAdapter);
    }

    void SettingsWindow::onScriptFilterChange(MyGUI::EditBox*)
    {
        renderScriptSettings();
    }

    void SettingsWindow::onScriptListSelection(MyGUI::ListBox*, size_t index)
    {
        mScriptAdapter->detach();
        mCurrentPage = static_cast<size_t>(-1);
        if (index < mScriptList->getItemCount())
        {
            mCurrentPage = *mScriptList->getItemDataAt<size_t>(index);
            LuaUi::attachPageAt(mCurrentPage, mScriptAdapter);
        }
    }

    void SettingsWindow::onRebindAction(MyGUI::Widget* sender)
    {
        int actionId = *sender->getUserData<int>();

        sender->castType<MyGUI::Button>()->setCaptionWithReplacing("#{Interface:None}");

        MWBase::Environment::get().getWindowManager()->staticMessageBox("#{OMWEngine:RebindAction}");
        MWBase::Environment::get().getWindowManager()->disallowMouse();

        MWBase::Environment::get().getInputManager()->enableDetectingBindingMode(actionId, mKeyboardMode);
    }

    void SettingsWindow::onInputTabMouseWheel(MyGUI::Widget* /*sender*/, int rel)
    {
        if (mControlsBox->getViewOffset().top + rel * 0.3f > 0)
            mControlsBox->setViewOffset(MyGUI::IntPoint(0, 0));
        else
            mControlsBox->setViewOffset(
                MyGUI::IntPoint(0, static_cast<int>(mControlsBox->getViewOffset().top + rel * 0.3f)));
    }

    void SettingsWindow::onResetDefaultBindings(MyGUI::Widget* /*sender*/)
    {
        ConfirmationDialog* dialog = MWBase::Environment::get().getWindowManager()->getConfirmationDialog();
        dialog->askForConfirmation("#{OMWEngine:ConfirmResetBindings}");
        dialog->eventOkClicked.clear();
        dialog->eventOkClicked += MyGUI::newDelegate(this, &SettingsWindow::onResetDefaultBindingsAccept);
        dialog->eventCancelClicked.clear();
    }

    void SettingsWindow::onResetDefaultBindingsAccept()
    {
        if (mKeyboardMode)
            MWBase::Environment::get().getInputManager()->resetToDefaultKeyBindings();
        else
            MWBase::Environment::get().getInputManager()->resetToDefaultControllerBindings();
        updateControlsBox();
    }

    void SettingsWindow::onOpen()
    {
        highlightCurrentResolution();
        updateControlsBox();
        updateLightSettings();
        updateWindowModeSettings();
        updateVSyncModeSettings();
        resetScrollbars();
        renderScriptSettings();
        MWBase::Environment::get().getWindowManager()->setKeyFocusWidget(mOkButton);
    }

    void SettingsWindow::onClose()
    {
        // Save user settings
        Settings::Manager::saveUser(mCfgMgr.getUserConfigPath() / "settings.cfg");
        MWBase::Environment::get().getLuaManager()->savePermanentStorage(mCfgMgr.getUserConfigPath());
        MWBase::Environment::get().getInputManager()->saveBindings();
    }

    void SettingsWindow::onWindowResize(MyGUI::Window* /*sender*/)
    {
        layoutControlsBox();
    }

    void SettingsWindow::computeMinimumWindowSize()
    {
        auto* window = mMainWidget->castType<MyGUI::Window>();
        auto minSize = window->getMinSize();

        // Window should be at minimum wide enough to show all tabs.
        int tabBarWidth = 0;
        for (uint32_t i = 0; i < mSettingsTab->getItemCount(); i++)
        {
            tabBarWidth += mSettingsTab->getButtonWidthAt(i);
        }

        // Need to include window margins
        int margins = mMainWidget->getWidth() - mSettingsTab->getWidth();
        int minimumWindowWidth = tabBarWidth + margins;

        if (minimumWindowWidth > minSize.width)
        {
            minSize.width = minimumWindowWidth;
            window->setMinSize(minSize);

            // Make a dummy call to setSize so MyGUI can apply any resize resulting from the change in MinSize
            mMainWidget->setSize(mMainWidget->getSize());
        }
    }

    void SettingsWindow::resetScrollbars()
    {
        mResolutionList->setScrollPosition(0);
        mControlsBox->setViewOffset(MyGUI::IntPoint(0, 0));
    }

    bool SettingsWindow::onControllerButtonEvent(const SDL_ControllerButtonEvent& arg)
    {
        if (arg.button == SDL_CONTROLLER_BUTTON_B)
        {
            onOkButtonClicked(mOkButton);
            return true;
        }
        else if (arg.button == SDL_CONTROLLER_BUTTON_LEFTSHOULDER)
        {
            size_t index = mSettingsTab->getIndexSelected();
            index = wrap(index, mSettingsTab->getItemCount(), -1);
            mSettingsTab->setIndexSelected(index);
            MWBase::Environment::get().getWindowManager()->playSound(ESM::RefId::stringRefId("Menu Click"));
            return true;
        }
        else if (arg.button == SDL_CONTROLLER_BUTTON_RIGHTSHOULDER)
        {
            size_t index = mSettingsTab->getIndexSelected();
            index = wrap(index, mSettingsTab->getItemCount(), 1);
            mSettingsTab->setIndexSelected(index);
            MWBase::Environment::get().getWindowManager()->playSound(ESM::RefId::stringRefId("Menu Click"));
            return true;
        }

        return false;
    }

}

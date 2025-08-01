#include "settingspage.hpp"

#include <array>
#include <cmath>
#include <string>

#include <QCompleter>
#include <QFileDialog>
#include <QString>

#include <components/config/gamesettings.hpp>

#include <components/settings/values.hpp>

#include "utils/openalutil.hpp"

namespace
{
    void loadSettingBool(const Settings::SettingValue<bool>& value, QCheckBox& checkbox)
    {
        checkbox.setCheckState(value ? Qt::Checked : Qt::Unchecked);
    }

    void saveSettingBool(const QCheckBox& checkbox, Settings::SettingValue<bool>& value)
    {
        value.set(checkbox.checkState() == Qt::Checked);
    }

    void loadSettingInt(const Settings::SettingValue<int>& value, QComboBox& comboBox)
    {
        comboBox.setCurrentIndex(value);
    }

    void loadSettingInt(const Settings::SettingValue<DetourNavigator::CollisionShapeType>& value, QComboBox& comboBox)
    {
        comboBox.setCurrentIndex(static_cast<int>(value.get()));
    }

    void saveSettingInt(const QComboBox& comboBox, Settings::SettingValue<int>& value)
    {
        value.set(comboBox.currentIndex());
    }

    void saveSettingInt(const QComboBox& comboBox, Settings::SettingValue<DetourNavigator::CollisionShapeType>& value)
    {
        value.set(static_cast<DetourNavigator::CollisionShapeType>(comboBox.currentIndex()));
    }

    void loadSettingInt(const Settings::SettingValue<int>& value, QSpinBox& spinBox)
    {
        spinBox.setValue(value);
    }

    void saveSettingInt(const QSpinBox& spinBox, Settings::SettingValue<int>& value)
    {
        value.set(spinBox.value());
    }

    int toIndex(Settings::HrtfMode value)
    {
        switch (value)
        {
            case Settings::HrtfMode::Auto:
                return 0;
            case Settings::HrtfMode::Disable:
                return 1;
            case Settings::HrtfMode::Enable:
                return 2;
        }
        return 0;
    }
}

Launcher::SettingsPage::SettingsPage(Config::GameSettings& gameSettings, QWidget* parent)
    : QWidget(parent)
    , mGameSettings(gameSettings)
{
    setObjectName("SettingsPage");
    setupUi(this);

    for (const std::string& name : Launcher::enumerateOpenALDevices())
    {
        audioDeviceSelectorComboBox->addItem(QString::fromStdString(name), QString::fromStdString(name));
    }
    for (const std::string& name : Launcher::enumerateOpenALDevicesHrtf())
    {
        hrtfProfileSelectorComboBox->addItem(QString::fromStdString(name), QString::fromStdString(name));
    }

    loadSettings();

    mCellNameCompleter.setModel(&mCellNameCompleterModel);
    startDefaultCharacterAtField->setCompleter(&mCellNameCompleter);
}

void Launcher::SettingsPage::loadCellsForAutocomplete(QStringList cellNames)
{
    // Update the list of suggestions for the "Start default character at" field
    mCellNameCompleterModel.setStringList(cellNames);
    mCellNameCompleter.setCompletionMode(QCompleter::PopupCompletion);
    mCellNameCompleter.setCaseSensitivity(Qt::CaseSensitivity::CaseInsensitive);
}

void Launcher::SettingsPage::on_skipMenuCheckBox_stateChanged(int state)
{
    startDefaultCharacterAtLabel->setEnabled(state == Qt::Checked);
    startDefaultCharacterAtField->setEnabled(state == Qt::Checked);
}

void Launcher::SettingsPage::on_runScriptAfterStartupBrowseButton_clicked()
{
    QString scriptFile = QFileDialog::getOpenFileName(
        this, QObject::tr("Select script file"), QDir::currentPath(), QString(tr("Text file (*.txt)")));

    if (scriptFile.isEmpty())
        return;

    QFileInfo info(scriptFile);

    if (!info.exists() || !info.isReadable())
        return;

    const QString path(QDir::toNativeSeparators(info.absoluteFilePath()));
    runScriptAfterStartupField->setText(path);
}

namespace
{
    constexpr double CellSizeInUnits = 8192;

    double convertToCells(double unitRadius)
    {
        return unitRadius / CellSizeInUnits;
    }

    int convertToUnits(double CellGridRadius)
    {
        return static_cast<int>(CellSizeInUnits * CellGridRadius);
    }
}

bool Launcher::SettingsPage::loadSettings()
{
    // Game mechanics
    {
        loadSettingBool(Settings::game().mCanLootDuringDeathAnimation, *canLootDuringDeathAnimationCheckBox);
        loadSettingBool(Settings::game().mFollowersAttackOnSight, *followersAttackOnSightCheckBox);
        loadSettingBool(Settings::game().mRebalanceSoulGemValues, *rebalanceSoulGemValuesCheckBox);
        loadSettingBool(Settings::game().mEnchantedWeaponsAreMagical, *enchantedWeaponsMagicalCheckBox);
        loadSettingBool(
            Settings::game().mBarterDispositionChangeIsPermanent, *permanentBarterDispositionChangeCheckBox);
        loadSettingBool(Settings::game().mClassicReflectedAbsorbSpellsBehavior, *classicReflectedAbsorbSpellsCheckBox);
        loadSettingBool(Settings::game().mClassicCalmSpellsBehavior, *classicCalmSpellsCheckBox);
        loadSettingBool(
            Settings::game().mOnlyAppropriateAmmunitionBypassesResistance, *requireAppropriateAmmunitionCheckBox);
        loadSettingBool(Settings::game().mUncappedDamageFatigue, *uncappedDamageFatigueCheckBox);
        loadSettingBool(Settings::game().mNormaliseRaceSpeed, *normaliseRaceSpeedCheckBox);
        loadSettingBool(Settings::game().mSwimUpwardCorrection, *swimUpwardCorrectionCheckBox);
        loadSettingBool(Settings::game().mNPCsAvoidCollisions, *avoidCollisionsCheckBox);
        loadSettingInt(Settings::game().mStrengthInfluencesHandToHand, *unarmedFactorsStrengthComboBox);
        loadSettingBool(Settings::game().mAlwaysAllowStealingFromKnockedOutActors, *stealingFromKnockedOutCheckBox);
        loadSettingBool(Settings::navigator().mEnable, *enableNavigatorCheckBox);
        loadSettingInt(Settings::physics().mAsyncNumThreads, *physicsThreadsSpinBox);
        loadSettingBool(
            Settings::game().mAllowActorsToFollowOverWaterSurface, *allowNPCToFollowOverWaterSurfaceCheckBox);
        loadSettingInt(Settings::game().mActorCollisionShapeType, *actorCollisonShapeTypeComboBox);
    }

    // Visuals
    {
        loadSettingBool(Settings::shaders().mAutoUseObjectNormalMaps, *autoUseObjectNormalMapsCheckBox);
        loadSettingBool(Settings::shaders().mAutoUseObjectSpecularMaps, *autoUseObjectSpecularMapsCheckBox);
        loadSettingBool(Settings::shaders().mAutoUseTerrainNormalMaps, *autoUseTerrainNormalMapsCheckBox);
        loadSettingBool(Settings::shaders().mAutoUseTerrainSpecularMaps, *autoUseTerrainSpecularMapsCheckBox);
        loadSettingBool(Settings::shaders().mApplyLightingToEnvironmentMaps, *bumpMapLocalLightingCheckBox);
        loadSettingBool(Settings::shaders().mSoftParticles, *softParticlesCheckBox);
        loadSettingBool(Settings::shaders().mAntialiasAlphaTest, *antialiasAlphaTestCheckBox);
        if (Settings::shaders().mAntialiasAlphaTest == 0)
            antialiasAlphaTestCheckBox->setCheckState(Qt::Unchecked);
        loadSettingBool(Settings::shaders().mAdjustCoverageForAlphaTest, *adjustCoverageForAlphaTestCheckBox);
        loadSettingBool(Settings::shaders().mWeatherParticleOcclusion, *weatherParticleOcclusionCheckBox);
        loadSettingBool(Settings::game().mUseMagicItemAnimations, *magicItemAnimationsCheckBox);
        connect(animSourcesCheckBox, &QCheckBox::toggled, this, &SettingsPage::slotAnimSourcesToggled);
        loadSettingBool(Settings::game().mUseAdditionalAnimSources, *animSourcesCheckBox);
        if (animSourcesCheckBox->checkState() != Qt::Unchecked)
        {
            loadSettingBool(Settings::game().mWeaponSheathing, *weaponSheathingCheckBox);
            loadSettingBool(Settings::game().mShieldSheathing, *shieldSheathingCheckBox);
        }
        loadSettingBool(Settings::game().mSmoothAnimTransitions, *smoothAnimTransitionsCheckBox);
        loadSettingBool(Settings::game().mTurnToMovementDirection, *turnToMovementDirectionCheckBox);
        loadSettingBool(Settings::game().mSmoothMovement, *smoothMovementCheckBox);
        loadSettingBool(Settings::game().mPlayerMovementIgnoresAnimation, *playerMovementIgnoresAnimationCheckBox);

        connect(distantLandCheckBox, &QCheckBox::toggled, this, &SettingsPage::slotDistantLandToggled);
        bool distantLandEnabled = Settings::terrain().mDistantTerrain && Settings::terrain().mObjectPaging;
        distantLandCheckBox->setCheckState(distantLandEnabled ? Qt::Checked : Qt::Unchecked);
        slotDistantLandToggled(distantLandEnabled);

        loadSettingBool(Settings::terrain().mObjectPagingActiveGrid, *activeGridObjectPagingCheckBox);
        viewingDistanceComboBox->setValue(convertToCells(Settings::camera().mViewingDistance));
        objectPagingMinSizeComboBox->setValue(Settings::terrain().mObjectPagingMinSize);

        loadSettingBool(Settings::game().mDayNightSwitches, *nightDaySwitchesCheckBox);

        connect(postprocessEnabledCheckBox, &QCheckBox::toggled, this, &SettingsPage::slotPostProcessToggled);
        loadSettingBool(Settings::postProcessing().mEnabled, *postprocessEnabledCheckBox);
        loadSettingBool(Settings::postProcessing().mTransparentPostpass, *postprocessTransparentPostpassCheckBox);
        postprocessHDRTimeComboBox->setValue(Settings::postProcessing().mAutoExposureSpeed);

        connect(skyBlendingCheckBox, &QCheckBox::toggled, this, &SettingsPage::slotSkyBlendingToggled);
        loadSettingBool(Settings::fog().mRadialFog, *radialFogCheckBox);
        loadSettingBool(Settings::fog().mExponentialFog, *exponentialFogCheckBox);
        loadSettingBool(Settings::fog().mSkyBlending, *skyBlendingCheckBox);
        skyBlendingStartComboBox->setValue(Settings::fog().mSkyBlendingStart);

        loadSettingBool(Settings::shadows().mActorShadows, *actorShadowsCheckBox);
        loadSettingBool(Settings::shadows().mPlayerShadows, *playerShadowsCheckBox);
        loadSettingBool(Settings::shadows().mTerrainShadows, *terrainShadowsCheckBox);
        loadSettingBool(Settings::shadows().mObjectShadows, *objectShadowsCheckBox);
        loadSettingBool(Settings::shadows().mEnableIndoorShadows, *indoorShadowsCheckBox);

        const auto& boundMethod = Settings::shadows().mComputeSceneBounds.get();
        if (boundMethod == "bounds")
            shadowComputeSceneBoundsComboBox->setCurrentIndex(0);
        else if (boundMethod == "primitives")
            shadowComputeSceneBoundsComboBox->setCurrentIndex(1);
        else
            shadowComputeSceneBoundsComboBox->setCurrentIndex(2);

        const int shadowDistLimit = Settings::shadows().mMaximumShadowMapDistance;
        if (shadowDistLimit > 0)
        {
            shadowDistanceCheckBox->setCheckState(Qt::Checked);
            shadowDistanceSpinBox->setValue(shadowDistLimit);
            shadowDistanceSpinBox->setEnabled(true);
            fadeStartSpinBox->setEnabled(true);
        }

        const float shadowFadeStart = Settings::shadows().mShadowFadeStart;
        if (shadowFadeStart != 0)
            fadeStartSpinBox->setValue(shadowFadeStart);

        const int shadowRes = Settings::shadows().mShadowMapResolution;
        int shadowResIndex = shadowResolutionComboBox->findText(QString::number(shadowRes));
        if (shadowResIndex != -1)
            shadowResolutionComboBox->setCurrentIndex(shadowResIndex);
        else
        {
            shadowResolutionComboBox->addItem(QString::number(shadowRes));
            shadowResolutionComboBox->setCurrentIndex(shadowResolutionComboBox->count() - 1);
        }

        connect(shadowDistanceCheckBox, &QCheckBox::toggled, this, &SettingsPage::slotShadowDistLimitToggled);

        int lightingMethod = 1;
        switch (Settings::shaders().mLightingMethod)
        {
            case SceneUtil::LightingMethod::FFP:
                lightingMethod = 0;
                break;
            case SceneUtil::LightingMethod::PerObjectUniform:
                lightingMethod = 1;
                break;
            case SceneUtil::LightingMethod::SingleUBO:
                lightingMethod = 2;
                break;
        }
        lightingMethodComboBox->setCurrentIndex(lightingMethod);
    }

    // Audio
    {
        const std::string& selectedAudioDevice = Settings::sound().mDevice;
        if (selectedAudioDevice.empty() == false)
        {
            int audioDeviceIndex = audioDeviceSelectorComboBox->findData(QString::fromStdString(selectedAudioDevice));
            if (audioDeviceIndex != -1)
            {
                audioDeviceSelectorComboBox->setCurrentIndex(audioDeviceIndex);
            }
        }
        enableHRTFComboBox->setCurrentIndex(toIndex(Settings::sound().mHrtfEnable));
        const std::string& selectedHRTFProfile = Settings::sound().mHrtf;
        if (selectedHRTFProfile.empty() == false)
        {
            int hrtfProfileIndex = hrtfProfileSelectorComboBox->findData(QString::fromStdString(selectedHRTFProfile));
            if (hrtfProfileIndex != -1)
            {
                hrtfProfileSelectorComboBox->setCurrentIndex(hrtfProfileIndex);
            }
        }
        loadSettingBool(Settings::sound().mCameraListener, *cameraListenerCheckBox);
    }

    // Interface Changes
    {
        loadSettingBool(Settings::game().mShowEffectDuration, *showEffectDurationCheckBox);
        loadSettingBool(Settings::game().mShowEnchantChance, *showEnchantChanceCheckBox);
        loadSettingBool(Settings::game().mShowMeleeInfo, *showMeleeInfoCheckBox);
        loadSettingBool(Settings::game().mShowProjectileDamage, *showProjectileDamageCheckBox);
        loadSettingBool(Settings::gui().mColorTopicEnable, *changeDialogTopicsCheckBox);
        showOwnedComboBox->setCurrentIndex(Settings::game().mShowOwned);
        loadSettingBool(Settings::gui().mStretchMenuBackground, *stretchBackgroundCheckBox);
        connect(controllerMenusCheckBox, &QCheckBox::toggled, this, &SettingsPage::slotControllerMenusToggled);
        loadSettingBool(Settings::gui().mControllerMenus, *controllerMenusCheckBox);
        loadSettingBool(Settings::gui().mControllerTooltips, *controllerMenuTooltipsCheckBox);
        loadSettingBool(Settings::map().mAllowZooming, *useZoomOnMapCheckBox);
        loadSettingBool(Settings::game().mGraphicHerbalism, *graphicHerbalismCheckBox);
        scalingSpinBox->setValue(Settings::gui().mScalingFactor);
        fontSizeSpinBox->setValue(Settings::gui().mFontSize);
    }

    // Bug fixes
    {
        loadSettingBool(Settings::game().mPreventMerchantEquipping, *preventMerchantEquippingCheckBox);
        loadSettingBool(
            Settings::game().mTrainersTrainingSkillsBasedOnBaseSkill, *trainersTrainingSkillsBasedOnBaseSkillCheckBox);
    }

    // Miscellaneous
    {
        // Saves
        loadSettingInt(Settings::saves().mMaxQuicksaves, *maximumQuicksavesComboBox);

        // Other Settings
        QString screenshotFormatString = QString::fromStdString(Settings::general().mScreenshotFormat).toUpper();
        if (screenshotFormatComboBox->findText(screenshotFormatString) == -1)
            screenshotFormatComboBox->addItem(screenshotFormatString);
        screenshotFormatComboBox->setCurrentIndex(screenshotFormatComboBox->findText(screenshotFormatString));

        loadSettingBool(Settings::general().mNotifyOnSavedScreenshot, *notifyOnSavedScreenshotCheckBox);
    }

    // Testing
    {
        loadSettingBool(Settings::input().mGrabCursor, *grabCursorCheckBox);

        bool skipMenu = mGameSettings.value("skip-menu").value.toInt() == 1;
        if (skipMenu)
        {
            skipMenuCheckBox->setCheckState(Qt::Checked);
        }
        startDefaultCharacterAtLabel->setEnabled(skipMenu);
        startDefaultCharacterAtField->setEnabled(skipMenu);

        startDefaultCharacterAtField->setText(mGameSettings.value("start").value);
        runScriptAfterStartupField->setText(mGameSettings.value("script-run").value);
    }
    return true;
}

void Launcher::SettingsPage::saveSettings()
{
    // Game mechanics
    {
        saveSettingBool(*canLootDuringDeathAnimationCheckBox, Settings::game().mCanLootDuringDeathAnimation);
        saveSettingBool(*followersAttackOnSightCheckBox, Settings::game().mFollowersAttackOnSight);
        saveSettingBool(*rebalanceSoulGemValuesCheckBox, Settings::game().mRebalanceSoulGemValues);
        saveSettingBool(*enchantedWeaponsMagicalCheckBox, Settings::game().mEnchantedWeaponsAreMagical);
        saveSettingBool(
            *permanentBarterDispositionChangeCheckBox, Settings::game().mBarterDispositionChangeIsPermanent);
        saveSettingBool(*classicReflectedAbsorbSpellsCheckBox, Settings::game().mClassicReflectedAbsorbSpellsBehavior);
        saveSettingBool(*classicCalmSpellsCheckBox, Settings::game().mClassicCalmSpellsBehavior);
        saveSettingBool(
            *requireAppropriateAmmunitionCheckBox, Settings::game().mOnlyAppropriateAmmunitionBypassesResistance);
        saveSettingBool(*uncappedDamageFatigueCheckBox, Settings::game().mUncappedDamageFatigue);
        saveSettingBool(*normaliseRaceSpeedCheckBox, Settings::game().mNormaliseRaceSpeed);
        saveSettingBool(*swimUpwardCorrectionCheckBox, Settings::game().mSwimUpwardCorrection);
        saveSettingBool(*avoidCollisionsCheckBox, Settings::game().mNPCsAvoidCollisions);
        saveSettingInt(*unarmedFactorsStrengthComboBox, Settings::game().mStrengthInfluencesHandToHand);
        saveSettingBool(*stealingFromKnockedOutCheckBox, Settings::game().mAlwaysAllowStealingFromKnockedOutActors);
        saveSettingBool(*enableNavigatorCheckBox, Settings::navigator().mEnable);
        saveSettingInt(*physicsThreadsSpinBox, Settings::physics().mAsyncNumThreads);
        saveSettingBool(
            *allowNPCToFollowOverWaterSurfaceCheckBox, Settings::game().mAllowActorsToFollowOverWaterSurface);
        saveSettingInt(*actorCollisonShapeTypeComboBox, Settings::game().mActorCollisionShapeType);
    }

    // Visuals
    {
        saveSettingBool(*autoUseObjectNormalMapsCheckBox, Settings::shaders().mAutoUseObjectNormalMaps);
        saveSettingBool(*autoUseObjectSpecularMapsCheckBox, Settings::shaders().mAutoUseObjectSpecularMaps);
        saveSettingBool(*autoUseTerrainNormalMapsCheckBox, Settings::shaders().mAutoUseTerrainNormalMaps);
        saveSettingBool(*autoUseTerrainSpecularMapsCheckBox, Settings::shaders().mAutoUseTerrainSpecularMaps);
        saveSettingBool(*bumpMapLocalLightingCheckBox, Settings::shaders().mApplyLightingToEnvironmentMaps);
        saveSettingBool(*radialFogCheckBox, Settings::fog().mRadialFog);
        saveSettingBool(*softParticlesCheckBox, Settings::shaders().mSoftParticles);
        saveSettingBool(*antialiasAlphaTestCheckBox, Settings::shaders().mAntialiasAlphaTest);
        saveSettingBool(*adjustCoverageForAlphaTestCheckBox, Settings::shaders().mAdjustCoverageForAlphaTest);
        saveSettingBool(*weatherParticleOcclusionCheckBox, Settings::shaders().mWeatherParticleOcclusion);
        saveSettingBool(*magicItemAnimationsCheckBox, Settings::game().mUseMagicItemAnimations);
        saveSettingBool(*animSourcesCheckBox, Settings::game().mUseAdditionalAnimSources);
        saveSettingBool(*weaponSheathingCheckBox, Settings::game().mWeaponSheathing);
        saveSettingBool(*shieldSheathingCheckBox, Settings::game().mShieldSheathing);
        saveSettingBool(*turnToMovementDirectionCheckBox, Settings::game().mTurnToMovementDirection);
        saveSettingBool(*smoothAnimTransitionsCheckBox, Settings::game().mSmoothAnimTransitions);
        saveSettingBool(*smoothMovementCheckBox, Settings::game().mSmoothMovement);
        saveSettingBool(*playerMovementIgnoresAnimationCheckBox, Settings::game().mPlayerMovementIgnoresAnimation);

        const bool wantDistantLand = distantLandCheckBox->checkState() == Qt::Checked;
        if (wantDistantLand != (Settings::terrain().mDistantTerrain && Settings::terrain().mObjectPaging))
        {
            Settings::terrain().mDistantTerrain.set(wantDistantLand);
            Settings::terrain().mObjectPaging.set(wantDistantLand);
        }

        saveSettingBool(*activeGridObjectPagingCheckBox, Settings::terrain().mObjectPagingActiveGrid);
        Settings::camera().mViewingDistance.set(convertToUnits(viewingDistanceComboBox->value()));
        Settings::terrain().mObjectPagingMinSize.set(objectPagingMinSizeComboBox->value());
        saveSettingBool(*nightDaySwitchesCheckBox, Settings::game().mDayNightSwitches);
        saveSettingBool(*postprocessEnabledCheckBox, Settings::postProcessing().mEnabled);
        saveSettingBool(*postprocessTransparentPostpassCheckBox, Settings::postProcessing().mTransparentPostpass);
        Settings::postProcessing().mAutoExposureSpeed.set(postprocessHDRTimeComboBox->value());
        saveSettingBool(*radialFogCheckBox, Settings::fog().mRadialFog);
        saveSettingBool(*exponentialFogCheckBox, Settings::fog().mExponentialFog);
        saveSettingBool(*skyBlendingCheckBox, Settings::fog().mSkyBlending);
        Settings::fog().mSkyBlendingStart.set(skyBlendingStartComboBox->value());

        static constexpr std::array<SceneUtil::LightingMethod, 3> lightingMethodMap = {
            SceneUtil::LightingMethod::FFP,
            SceneUtil::LightingMethod::PerObjectUniform,
            SceneUtil::LightingMethod::SingleUBO,
        };
        Settings::shaders().mLightingMethod.set(lightingMethodMap[lightingMethodComboBox->currentIndex()]);

        const int cShadowDist
            = shadowDistanceCheckBox->checkState() != Qt::Unchecked ? shadowDistanceSpinBox->value() : 0;
        Settings::shadows().mMaximumShadowMapDistance.set(cShadowDist);
        const float cFadeStart = fadeStartSpinBox->value();
        if (cShadowDist > 0)
            Settings::shadows().mShadowFadeStart.set(cFadeStart);

        const bool cActorShadows = actorShadowsCheckBox->checkState() != Qt::Unchecked;
        const bool cObjectShadows = objectShadowsCheckBox->checkState() != Qt::Unchecked;
        const bool cTerrainShadows = terrainShadowsCheckBox->checkState() != Qt::Unchecked;
        const bool cPlayerShadows = playerShadowsCheckBox->checkState() != Qt::Unchecked;
        if (cActorShadows || cObjectShadows || cTerrainShadows || cPlayerShadows)
        {
            Settings::shadows().mEnableShadows.set(true);
            Settings::shadows().mActorShadows.set(cActorShadows);
            Settings::shadows().mPlayerShadows.set(cPlayerShadows);
            Settings::shadows().mObjectShadows.set(cObjectShadows);
            Settings::shadows().mTerrainShadows.set(cTerrainShadows);
        }
        else
        {
            Settings::shadows().mEnableShadows.set(false);
            Settings::shadows().mActorShadows.set(false);
            Settings::shadows().mPlayerShadows.set(false);
            Settings::shadows().mObjectShadows.set(false);
            Settings::shadows().mTerrainShadows.set(false);
        }

        Settings::shadows().mEnableIndoorShadows.set(indoorShadowsCheckBox->checkState() != Qt::Unchecked);
        Settings::shadows().mShadowMapResolution.set(shadowResolutionComboBox->currentText().toInt());

        auto index = shadowComputeSceneBoundsComboBox->currentIndex();
        if (index == 0)
            Settings::shadows().mComputeSceneBounds.set("bounds");
        else if (index == 1)
            Settings::shadows().mComputeSceneBounds.set("primitives");
        else
            Settings::shadows().mComputeSceneBounds.set("none");
    }

    // Audio
    {
        if (audioDeviceSelectorComboBox->currentIndex() != 0)
            Settings::sound().mDevice.set(audioDeviceSelectorComboBox->currentText().toStdString());
        else
            Settings::sound().mDevice.set({});

        static constexpr std::array<Settings::HrtfMode, 3> hrtfModes{
            Settings::HrtfMode::Auto,
            Settings::HrtfMode::Disable,
            Settings::HrtfMode::Enable,
        };
        Settings::sound().mHrtfEnable.set(hrtfModes[enableHRTFComboBox->currentIndex()]);

        if (hrtfProfileSelectorComboBox->currentIndex() != 0)
            Settings::sound().mHrtf.set(hrtfProfileSelectorComboBox->currentText().toStdString());
        else
            Settings::sound().mHrtf.set({});

        const bool cCameraListener = cameraListenerCheckBox->checkState() != Qt::Unchecked;
        Settings::sound().mCameraListener.set(cCameraListener);
    }

    // Interface Changes
    {
        saveSettingBool(*showEffectDurationCheckBox, Settings::game().mShowEffectDuration);
        saveSettingBool(*showEnchantChanceCheckBox, Settings::game().mShowEnchantChance);
        saveSettingBool(*showMeleeInfoCheckBox, Settings::game().mShowMeleeInfo);
        saveSettingBool(*showProjectileDamageCheckBox, Settings::game().mShowProjectileDamage);
        saveSettingBool(*changeDialogTopicsCheckBox, Settings::gui().mColorTopicEnable);
        saveSettingInt(*showOwnedComboBox, Settings::game().mShowOwned);
        saveSettingBool(*stretchBackgroundCheckBox, Settings::gui().mStretchMenuBackground);
        saveSettingBool(*controllerMenusCheckBox, Settings::gui().mControllerMenus);
        saveSettingBool(*controllerMenuTooltipsCheckBox, Settings::gui().mControllerTooltips);
        saveSettingBool(*useZoomOnMapCheckBox, Settings::map().mAllowZooming);
        saveSettingBool(*graphicHerbalismCheckBox, Settings::game().mGraphicHerbalism);
        Settings::gui().mScalingFactor.set(scalingSpinBox->value());
        Settings::gui().mFontSize.set(fontSizeSpinBox->value());
    }

    // Bug fixes
    {
        saveSettingBool(*preventMerchantEquippingCheckBox, Settings::game().mPreventMerchantEquipping);
        saveSettingBool(
            *trainersTrainingSkillsBasedOnBaseSkillCheckBox, Settings::game().mTrainersTrainingSkillsBasedOnBaseSkill);
    }

    // Miscellaneous
    {
        // Saves Settings
        saveSettingInt(*maximumQuicksavesComboBox, Settings::saves().mMaxQuicksaves);

        // Other Settings
        Settings::general().mScreenshotFormat.set(screenshotFormatComboBox->currentText().toLower().toStdString());
        saveSettingBool(*notifyOnSavedScreenshotCheckBox, Settings::general().mNotifyOnSavedScreenshot);
    }

    // Testing
    {
        saveSettingBool(*grabCursorCheckBox, Settings::input().mGrabCursor);

        int skipMenu = skipMenuCheckBox->checkState() == Qt::Checked;
        if (skipMenu != mGameSettings.value("skip-menu").value.toInt())
            mGameSettings.setValue("skip-menu", { QString::number(skipMenu) });

        QString startCell = startDefaultCharacterAtField->text();
        if (startCell != mGameSettings.value("start").value)
        {
            mGameSettings.setValue("start", { startCell });
        }
        QString scriptRun = runScriptAfterStartupField->text();
        if (scriptRun != mGameSettings.value("script-run").value)
            mGameSettings.setValue("script-run", { scriptRun });
    }
}

void Launcher::SettingsPage::slotLoadedCellsChanged(QStringList cellNames)
{
    loadCellsForAutocomplete(std::move(cellNames));
}

void Launcher::SettingsPage::slotAnimSourcesToggled(bool checked)
{
    weaponSheathingCheckBox->setEnabled(checked);
    shieldSheathingCheckBox->setEnabled(checked);
    if (!checked)
    {
        weaponSheathingCheckBox->setCheckState(Qt::Unchecked);
        shieldSheathingCheckBox->setCheckState(Qt::Unchecked);
    }
}

void Launcher::SettingsPage::slotControllerMenusToggled(bool checked)
{
    controllerMenuTooltipsCheckBox->setEnabled(checked);
}

void Launcher::SettingsPage::slotPostProcessToggled(bool checked)
{
    postprocessTransparentPostpassCheckBox->setEnabled(checked);
    postprocessHDRTimeComboBox->setEnabled(checked);
    postprocessHDRTimeLabel->setEnabled(checked);
}

void Launcher::SettingsPage::slotSkyBlendingToggled(bool checked)
{
    skyBlendingStartComboBox->setEnabled(checked);
    skyBlendingStartLabel->setEnabled(checked);
}

void Launcher::SettingsPage::slotShadowDistLimitToggled(bool checked)
{
    shadowDistanceSpinBox->setEnabled(checked);
    fadeStartSpinBox->setEnabled(checked);
}

void Launcher::SettingsPage::slotDistantLandToggled(bool checked)
{
    activeGridObjectPagingCheckBox->setEnabled(checked);
    objectPagingMinSizeComboBox->setEnabled(checked);
}

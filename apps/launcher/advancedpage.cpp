#include "advancedpage.hpp"

#include <array>

#include <components/config/gamesettings.hpp>
#include <components/config/launchersettings.hpp>
#include <QFileDialog>
#include <QCompleter>
#include <QProxyStyle>
#include <QString>
#include <components/contentselector/view/contentselector.hpp>
#include <components/contentselector/model/esmfile.hpp>

#include <cmath>

#include "utils/openalutil.hpp"

Launcher::AdvancedPage::AdvancedPage(Config::GameSettings &gameSettings,
                                     Settings::Manager &engineSettings, QWidget *parent)
        : QWidget(parent)
        , mGameSettings(gameSettings)
        , mEngineSettings(engineSettings)
{
    setObjectName ("AdvancedPage");
    setupUi(this);

    for(const char * name : Launcher::enumerateOpenALDevices())
    {
        audioDeviceSelectorComboBox->addItem(QString::fromUtf8(name), QString::fromUtf8(name));
    }
    for(const char * name : Launcher::enumerateOpenALDevicesHrtf())
    {
        hrtfProfileSelectorComboBox->addItem(QString::fromUtf8(name), QString::fromUtf8(name));
    }

    loadSettings();

    mCellNameCompleter.setModel(&mCellNameCompleterModel);
    startDefaultCharacterAtField->setCompleter(&mCellNameCompleter);
}

void Launcher::AdvancedPage::loadCellsForAutocomplete(QStringList cellNames) {
    // Update the list of suggestions for the "Start default character at" field
    mCellNameCompleterModel.setStringList(cellNames);
    mCellNameCompleter.setCompletionMode(QCompleter::PopupCompletion);
    mCellNameCompleter.setCaseSensitivity(Qt::CaseSensitivity::CaseInsensitive);
}

void Launcher::AdvancedPage::on_skipMenuCheckBox_stateChanged(int state) {
    startDefaultCharacterAtLabel->setEnabled(state == Qt::Checked);
    startDefaultCharacterAtField->setEnabled(state == Qt::Checked);
}

void Launcher::AdvancedPage::on_runScriptAfterStartupBrowseButton_clicked()
{
    QString scriptFile = QFileDialog::getOpenFileName(
            this,
            QObject::tr("Select script file"),
            QDir::currentPath(),
            QString(tr("Text file (*.txt)")));

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
        return std::round((unitRadius + 1024) / CellSizeInUnits);
    }

    double convertToUnits(double CellGridRadius)
    {
        return CellSizeInUnits * CellGridRadius - 1024;
    }
}

bool Launcher::AdvancedPage::loadSettings()
{
    // Game mechanics
    {
        loadSettingBool(toggleSneakCheckBox, "toggle sneak", "Input");
        loadSettingBool(canLootDuringDeathAnimationCheckBox, "can loot during death animation", "Game");
        loadSettingBool(followersAttackOnSightCheckBox, "followers attack on sight", "Game");
        loadSettingBool(rebalanceSoulGemValuesCheckBox, "rebalance soul gem values", "Game");
        loadSettingBool(enchantedWeaponsMagicalCheckBox, "enchanted weapons are magical", "Game");
        loadSettingBool(permanentBarterDispositionChangeCheckBox, "barter disposition change is permanent", "Game");
        loadSettingBool(classicReflectedAbsorbSpellsCheckBox, "classic reflected absorb spells behavior", "Game");
        loadSettingBool(requireAppropriateAmmunitionCheckBox, "only appropriate ammunition bypasses resistance", "Game");
        loadSettingBool(uncappedDamageFatigueCheckBox, "uncapped damage fatigue", "Game");
        loadSettingBool(normaliseRaceSpeedCheckBox, "normalise race speed", "Game");
        loadSettingBool(swimUpwardCorrectionCheckBox, "swim upward correction", "Game");
        loadSettingBool(avoidCollisionsCheckBox, "NPCs avoid collisions", "Game");
        int unarmedFactorsStrengthIndex = mEngineSettings.getInt("strength influences hand to hand", "Game");
        if (unarmedFactorsStrengthIndex >= 0 && unarmedFactorsStrengthIndex <= 2)
            unarmedFactorsStrengthComboBox->setCurrentIndex(unarmedFactorsStrengthIndex);
        loadSettingBool(stealingFromKnockedOutCheckBox, "always allow stealing from knocked out actors", "Game");
        loadSettingBool(enableNavigatorCheckBox, "enable", "Navigator");
        int numPhysicsThreads = mEngineSettings.getInt("async num threads", "Physics");
        if (numPhysicsThreads >= 0)
            physicsThreadsSpinBox->setValue(numPhysicsThreads);
        loadSettingBool(allowNPCToFollowOverWaterSurfaceCheckBox, "allow actors to follow over water surface", "Game");
    }

    // Visuals
    {
        loadSettingBool(autoUseObjectNormalMapsCheckBox, "auto use object normal maps", "Shaders");
        loadSettingBool(autoUseObjectSpecularMapsCheckBox, "auto use object specular maps", "Shaders");
        loadSettingBool(autoUseTerrainNormalMapsCheckBox, "auto use terrain normal maps", "Shaders");
        loadSettingBool(autoUseTerrainSpecularMapsCheckBox, "auto use terrain specular maps", "Shaders");
        loadSettingBool(bumpMapLocalLightingCheckBox, "apply lighting to environment maps", "Shaders");
        loadSettingBool(radialFogCheckBox, "radial fog", "Shaders");
        loadSettingBool(magicItemAnimationsCheckBox, "use magic item animations", "Game");
        connect(animSourcesCheckBox, SIGNAL(toggled(bool)), this, SLOT(slotAnimSourcesToggled(bool)));
        loadSettingBool(animSourcesCheckBox, "use additional anim sources", "Game");
        if (animSourcesCheckBox->checkState() != Qt::Unchecked)
        {
            loadSettingBool(weaponSheathingCheckBox, "weapon sheathing", "Game");
            loadSettingBool(shieldSheathingCheckBox, "shield sheathing", "Game");
        }
        loadSettingBool(turnToMovementDirectionCheckBox, "turn to movement direction", "Game");
        loadSettingBool(smoothMovementCheckBox, "smooth movement", "Game");

        const bool distantTerrain = mEngineSettings.getBool("distant terrain", "Terrain");
        const bool objectPaging = mEngineSettings.getBool("object paging", "Terrain");
        if (distantTerrain && objectPaging) {
            distantLandCheckBox->setCheckState(Qt::Checked);
        }

        loadSettingBool(activeGridObjectPagingCheckBox, "object paging active grid", "Terrain");
        viewingDistanceComboBox->setValue(convertToCells(mEngineSettings.getInt("viewing distance", "Camera")));

        int lightingMethod = 1;
        if (mEngineSettings.getString("lighting method", "Shaders") == "legacy")
            lightingMethod = 0;
        else if (mEngineSettings.getString("lighting method", "Shaders") == "shaders")
            lightingMethod = 2;
        lightingMethodComboBox->setCurrentIndex(lightingMethod);
    }

    // Audio
    {
        std::string selectedAudioDevice = mEngineSettings.getString("device", "Sound");
        if (selectedAudioDevice.empty() == false)
        {
            int audioDeviceIndex = audioDeviceSelectorComboBox->findData(QString::fromStdString(selectedAudioDevice));
            if (audioDeviceIndex != -1)
            {
                audioDeviceSelectorComboBox->setCurrentIndex(audioDeviceIndex);
            }
        }
        int hrtfEnabledIndex = mEngineSettings.getInt("hrtf enable", "Sound");
        if (hrtfEnabledIndex >= -1 && hrtfEnabledIndex <= 1)
        {
            enableHRTFComboBox->setCurrentIndex(hrtfEnabledIndex + 1);
        }
        std::string selectedHRTFProfile = mEngineSettings.getString("hrtf", "Sound");
        if (selectedHRTFProfile.empty() == false)
        {
            int hrtfProfileIndex = hrtfProfileSelectorComboBox->findData(QString::fromStdString(selectedHRTFProfile));
            if (hrtfProfileIndex != -1)
            {
                hrtfProfileSelectorComboBox->setCurrentIndex(hrtfProfileIndex);
            }
        }
    }


    // Camera
    {
        loadSettingBool(viewOverShoulderCheckBox, "view over shoulder", "Camera");
        connect(viewOverShoulderCheckBox, SIGNAL(toggled(bool)), this, SLOT(slotViewOverShoulderToggled(bool)));
        viewOverShoulderVerticalLayout->setEnabled(viewOverShoulderCheckBox->checkState());
        loadSettingBool(autoSwitchShoulderCheckBox, "auto switch shoulder", "Camera");
        loadSettingBool(previewIfStandStillCheckBox, "preview if stand still", "Camera");
        loadSettingBool(deferredPreviewRotationCheckBox, "deferred preview rotation", "Camera");
        loadSettingBool(headBobbingCheckBox, "head bobbing", "Camera");
        defaultShoulderComboBox->setCurrentIndex(
            mEngineSettings.getVector2("view over shoulder offset", "Camera").x() >= 0 ? 0 : 1);
    }

    // Interface Changes
    {
        loadSettingBool(showEffectDurationCheckBox, "show effect duration", "Game");
        loadSettingBool(showEnchantChanceCheckBox, "show enchant chance", "Game");
        loadSettingBool(showMeleeInfoCheckBox, "show melee info", "Game");
        loadSettingBool(showProjectileDamageCheckBox, "show projectile damage", "Game");
        loadSettingBool(changeDialogTopicsCheckBox, "color topic enable", "GUI");
        int showOwnedIndex = mEngineSettings.getInt("show owned", "Game");
        // Match the index with the option (only 0, 1, 2, or 3 are valid). Will default to 0 if invalid.
        if (showOwnedIndex >= 0 && showOwnedIndex <= 3)
            showOwnedComboBox->setCurrentIndex(showOwnedIndex);
        loadSettingBool(stretchBackgroundCheckBox, "stretch menu background", "GUI");
        loadSettingBool(graphicHerbalismCheckBox, "graphic herbalism", "Game");
        scalingSpinBox->setValue(mEngineSettings.getFloat("scaling factor", "GUI"));
    }

    // Bug fixes
    {
        loadSettingBool(preventMerchantEquippingCheckBox, "prevent merchant equipping", "Game");
        loadSettingBool(trainersTrainingSkillsBasedOnBaseSkillCheckBox, "trainers training skills based on base skill", "Game");
    }

    // Miscellaneous
    {
        // Saves
        loadSettingBool(timePlayedCheckbox, "timeplayed", "Saves");
        maximumQuicksavesComboBox->setValue(mEngineSettings.getInt("max quicksaves", "Saves"));

        // Other Settings
        QString screenshotFormatString = QString::fromStdString(mEngineSettings.getString("screenshot format", "General")).toUpper();
        if (screenshotFormatComboBox->findText(screenshotFormatString) == -1)
            screenshotFormatComboBox->addItem(screenshotFormatString);
        screenshotFormatComboBox->setCurrentIndex(screenshotFormatComboBox->findText(screenshotFormatString));
    }

    // Testing
    {
        loadSettingBool(grabCursorCheckBox, "grab cursor", "Input");

        bool skipMenu = mGameSettings.value("skip-menu").toInt() == 1;
        if (skipMenu)
        {
            skipMenuCheckBox->setCheckState(Qt::Checked);
        }
        startDefaultCharacterAtLabel->setEnabled(skipMenu);
        startDefaultCharacterAtField->setEnabled(skipMenu);

        startDefaultCharacterAtField->setText(mGameSettings.value("start"));
        runScriptAfterStartupField->setText(mGameSettings.value("script-run"));
    }
    return true;
}

void Launcher::AdvancedPage::saveSettings()
{
    // Game mechanics
    {
        saveSettingBool(toggleSneakCheckBox, "toggle sneak", "Input");
        saveSettingBool(canLootDuringDeathAnimationCheckBox, "can loot during death animation", "Game");
        saveSettingBool(followersAttackOnSightCheckBox, "followers attack on sight", "Game");
        saveSettingBool(rebalanceSoulGemValuesCheckBox, "rebalance soul gem values", "Game");
        saveSettingBool(enchantedWeaponsMagicalCheckBox, "enchanted weapons are magical", "Game");
        saveSettingBool(permanentBarterDispositionChangeCheckBox, "barter disposition change is permanent", "Game");
        saveSettingBool(classicReflectedAbsorbSpellsCheckBox, "classic reflected absorb spells behavior", "Game");
        saveSettingBool(requireAppropriateAmmunitionCheckBox, "only appropriate ammunition bypasses resistance", "Game");
        saveSettingBool(uncappedDamageFatigueCheckBox, "uncapped damage fatigue", "Game");
        saveSettingBool(normaliseRaceSpeedCheckBox, "normalise race speed", "Game");
        saveSettingBool(swimUpwardCorrectionCheckBox, "swim upward correction", "Game");
        saveSettingBool(avoidCollisionsCheckBox, "NPCs avoid collisions", "Game");
        int unarmedFactorsStrengthIndex = unarmedFactorsStrengthComboBox->currentIndex();
        if (unarmedFactorsStrengthIndex != mEngineSettings.getInt("strength influences hand to hand", "Game"))
            mEngineSettings.setInt("strength influences hand to hand", "Game", unarmedFactorsStrengthIndex);
        saveSettingBool(stealingFromKnockedOutCheckBox, "always allow stealing from knocked out actors", "Game");
        saveSettingBool(enableNavigatorCheckBox, "enable", "Navigator");
        int numPhysicsThreads = physicsThreadsSpinBox->value();
        if (numPhysicsThreads != mEngineSettings.getInt("async num threads", "Physics"))
            mEngineSettings.setInt("async num threads", "Physics", numPhysicsThreads);
    }

    // Visuals
    {
        saveSettingBool(autoUseObjectNormalMapsCheckBox, "auto use object normal maps", "Shaders");
        saveSettingBool(autoUseObjectSpecularMapsCheckBox, "auto use object specular maps", "Shaders");
        saveSettingBool(autoUseTerrainNormalMapsCheckBox, "auto use terrain normal maps", "Shaders");
        saveSettingBool(autoUseTerrainSpecularMapsCheckBox, "auto use terrain specular maps", "Shaders");
        saveSettingBool(bumpMapLocalLightingCheckBox, "apply lighting to environment maps", "Shaders");
        saveSettingBool(radialFogCheckBox, "radial fog", "Shaders");
        saveSettingBool(magicItemAnimationsCheckBox, "use magic item animations", "Game");
        saveSettingBool(animSourcesCheckBox, "use additional anim sources", "Game");
        saveSettingBool(weaponSheathingCheckBox, "weapon sheathing", "Game");
        saveSettingBool(shieldSheathingCheckBox, "shield sheathing", "Game");
        saveSettingBool(turnToMovementDirectionCheckBox, "turn to movement direction", "Game");
        saveSettingBool(smoothMovementCheckBox, "smooth movement", "Game");

        const bool distantTerrain = mEngineSettings.getBool("distant terrain", "Terrain");
        const bool objectPaging = mEngineSettings.getBool("object paging", "Terrain");
        const bool wantDistantLand = distantLandCheckBox->checkState();
        if (wantDistantLand != (distantTerrain && objectPaging)) {
            mEngineSettings.setBool("distant terrain", "Terrain", wantDistantLand);
            mEngineSettings.setBool("object paging", "Terrain", wantDistantLand);
        }

        saveSettingBool(activeGridObjectPagingCheckBox, "object paging active grid", "Terrain");
        double viewingDistance = viewingDistanceComboBox->value();
        if (viewingDistance != convertToCells(mEngineSettings.getInt("viewing distance", "Camera")))
        {
            mEngineSettings.setInt("viewing distance", "Camera", convertToUnits(viewingDistance));
        }

        static std::array<std::string, 3> lightingMethodMap = {"legacy", "shaders compatibility", "shaders"};
        mEngineSettings.setString("lighting method", "Shaders", lightingMethodMap[lightingMethodComboBox->currentIndex()]);
    }
    
    // Audio
    {
        int audioDeviceIndex = audioDeviceSelectorComboBox->currentIndex();
        if (audioDeviceIndex != 0)
        {
            mEngineSettings.setString("device", "Sound", audioDeviceSelectorComboBox->currentText().toUtf8().constData());
        } 
        else 
        {
            mEngineSettings.setString("device", "Sound", "");
        }
        int hrtfEnabledIndex = enableHRTFComboBox->currentIndex() - 1;
        if (hrtfEnabledIndex != mEngineSettings.getInt("hrtf enable", "Sound"))
        {
            mEngineSettings.setInt("hrtf enable", "Sound", hrtfEnabledIndex);
        }
        int selectedHRTFProfileIndex = hrtfProfileSelectorComboBox->currentIndex();
        if (selectedHRTFProfileIndex != 0)
        {
            mEngineSettings.setString("hrtf", "Sound", hrtfProfileSelectorComboBox->currentText().toUtf8().constData());
        }
        else 
        {
            mEngineSettings.setString("hrtf", "Sound", "");
        }
    }

    // Camera
    {
        saveSettingBool(viewOverShoulderCheckBox, "view over shoulder", "Camera");
        saveSettingBool(autoSwitchShoulderCheckBox, "auto switch shoulder", "Camera");
        saveSettingBool(previewIfStandStillCheckBox, "preview if stand still", "Camera");
        saveSettingBool(deferredPreviewRotationCheckBox, "deferred preview rotation", "Camera");
        saveSettingBool(headBobbingCheckBox, "head bobbing", "Camera");

        osg::Vec2f shoulderOffset = mEngineSettings.getVector2("view over shoulder offset", "Camera");
        if (defaultShoulderComboBox->currentIndex() != (shoulderOffset.x() >= 0 ? 0 : 1))
        {
            if (defaultShoulderComboBox->currentIndex() == 0)
                shoulderOffset.x() = std::abs(shoulderOffset.x());
            else
                shoulderOffset.x() = -std::abs(shoulderOffset.x());
            mEngineSettings.setVector2("view over shoulder offset", "Camera", shoulderOffset);
        }
    }

    // Interface Changes
    {
        saveSettingBool(showEffectDurationCheckBox, "show effect duration", "Game");
        saveSettingBool(showEnchantChanceCheckBox, "show enchant chance", "Game");
        saveSettingBool(showMeleeInfoCheckBox, "show melee info", "Game");
        saveSettingBool(showProjectileDamageCheckBox, "show projectile damage", "Game");
        saveSettingBool(changeDialogTopicsCheckBox, "color topic enable", "GUI");
        int showOwnedCurrentIndex = showOwnedComboBox->currentIndex();
        if (showOwnedCurrentIndex != mEngineSettings.getInt("show owned", "Game"))
            mEngineSettings.setInt("show owned", "Game", showOwnedCurrentIndex);
        saveSettingBool(stretchBackgroundCheckBox, "stretch menu background", "GUI");
        saveSettingBool(graphicHerbalismCheckBox, "graphic herbalism", "Game");
        float uiScalingFactor = scalingSpinBox->value();
        if (uiScalingFactor != mEngineSettings.getFloat("scaling factor", "GUI"))
            mEngineSettings.setFloat("scaling factor", "GUI", uiScalingFactor);
    }

    // Bug fixes
    {
        saveSettingBool(preventMerchantEquippingCheckBox, "prevent merchant equipping", "Game");
        saveSettingBool(trainersTrainingSkillsBasedOnBaseSkillCheckBox, "trainers training skills based on base skill", "Game");
    }

    // Miscellaneous
    {
        // Saves Settings
        saveSettingBool(timePlayedCheckbox, "timeplayed", "Saves");
        int maximumQuicksaves = maximumQuicksavesComboBox->value();
        if (maximumQuicksaves != mEngineSettings.getInt("max quicksaves", "Saves"))
        {
            mEngineSettings.setInt("max quicksaves", "Saves", maximumQuicksaves);
        }

        // Other Settings
        std::string screenshotFormatString = screenshotFormatComboBox->currentText().toLower().toStdString();
        if (screenshotFormatString != mEngineSettings.getString("screenshot format", "General"))
            mEngineSettings.setString("screenshot format", "General", screenshotFormatString);
    }

    // Testing
    {
        saveSettingBool(grabCursorCheckBox, "grab cursor", "Input");

        int skipMenu = skipMenuCheckBox->checkState() == Qt::Checked;
        if (skipMenu != mGameSettings.value("skip-menu").toInt())
            mGameSettings.setValue("skip-menu", QString::number(skipMenu));

        QString startCell = startDefaultCharacterAtField->text();
        if (startCell != mGameSettings.value("start"))
        {
            mGameSettings.setValue("start", startCell);
        }
        QString scriptRun = runScriptAfterStartupField->text();
        if (scriptRun != mGameSettings.value("script-run"))
            mGameSettings.setValue("script-run", scriptRun);
    }
}

void Launcher::AdvancedPage::loadSettingBool(QCheckBox *checkbox, const std::string &setting, const std::string &group)
{
    if (mEngineSettings.getBool(setting, group))
        checkbox->setCheckState(Qt::Checked);
}

void Launcher::AdvancedPage::saveSettingBool(QCheckBox *checkbox, const std::string &setting, const std::string &group)
{
    bool cValue = checkbox->checkState();
    if (cValue != mEngineSettings.getBool(setting, group))
        mEngineSettings.setBool(setting, group, cValue);
}

void Launcher::AdvancedPage::slotLoadedCellsChanged(QStringList cellNames)
{
    loadCellsForAutocomplete(cellNames);
}

void Launcher::AdvancedPage::slotAnimSourcesToggled(bool checked)
{
    weaponSheathingCheckBox->setEnabled(checked);
    shieldSheathingCheckBox->setEnabled(checked);
    if (!checked)
    {
        weaponSheathingCheckBox->setCheckState(Qt::Unchecked);
        shieldSheathingCheckBox->setCheckState(Qt::Unchecked);
    }
}

void Launcher::AdvancedPage::slotViewOverShoulderToggled(bool checked)
{
    viewOverShoulderVerticalLayout->setEnabled(viewOverShoulderCheckBox->checkState());
}

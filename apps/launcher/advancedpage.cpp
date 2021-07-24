#include "advancedpage.hpp"

#include <array>

#include <components/config/gamesettings.hpp>
#include <QFileDialog>
#include <QCompleter>
#include <QString>
#include <components/contentselector/view/contentselector.hpp>
#include <components/contentselector/model/esmfile.hpp>

#include <cmath>

#include "utils/openalutil.hpp"

Launcher::AdvancedPage::AdvancedPage(Config::GameSettings &gameSettings, QWidget *parent)
        : QWidget(parent)
        , mGameSettings(gameSettings)
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
        int unarmedFactorsStrengthIndex = Settings::Manager::getInt("strength influences hand to hand", "Game");
        if (unarmedFactorsStrengthIndex >= 0 && unarmedFactorsStrengthIndex <= 2)
            unarmedFactorsStrengthComboBox->setCurrentIndex(unarmedFactorsStrengthIndex);
        loadSettingBool(stealingFromKnockedOutCheckBox, "always allow stealing from knocked out actors", "Game");
        loadSettingBool(enableNavigatorCheckBox, "enable", "Navigator");
        int numPhysicsThreads = Settings::Manager::getInt("async num threads", "Physics");
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

        const bool distantTerrain = Settings::Manager::getBool("distant terrain", "Terrain");
        const bool objectPaging = Settings::Manager::getBool("object paging", "Terrain");
        if (distantTerrain && objectPaging) {
            distantLandCheckBox->setCheckState(Qt::Checked);
        }

        loadSettingBool(activeGridObjectPagingCheckBox, "object paging active grid", "Terrain");
        viewingDistanceComboBox->setValue(convertToCells(Settings::Manager::getInt("viewing distance", "Camera")));
        objectPagingMinSizeComboBox->setValue(Settings::Manager::getDouble("object paging min size", "Terrain"));
    }

    // Audio
    {
        std::string selectedAudioDevice = Settings::Manager::getString("device", "Sound");
        if (selectedAudioDevice.empty() == false)
        {
            int audioDeviceIndex = audioDeviceSelectorComboBox->findData(QString::fromStdString(selectedAudioDevice));
            if (audioDeviceIndex != -1)
            {
                audioDeviceSelectorComboBox->setCurrentIndex(audioDeviceIndex);
            }
        }
        int hrtfEnabledIndex = Settings::Manager::getInt("hrtf enable", "Sound");
        if (hrtfEnabledIndex >= -1 && hrtfEnabledIndex <= 1)
        {
            enableHRTFComboBox->setCurrentIndex(hrtfEnabledIndex + 1);
        }
        std::string selectedHRTFProfile = Settings::Manager::getString("hrtf", "Sound");
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
            Settings::Manager::getVector2("view over shoulder offset", "Camera").x() >= 0 ? 0 : 1);
    }

    // Interface Changes
    {
        loadSettingBool(showEffectDurationCheckBox, "show effect duration", "Game");
        loadSettingBool(showEnchantChanceCheckBox, "show enchant chance", "Game");
        loadSettingBool(showMeleeInfoCheckBox, "show melee info", "Game");
        loadSettingBool(showProjectileDamageCheckBox, "show projectile damage", "Game");
        loadSettingBool(changeDialogTopicsCheckBox, "color topic enable", "GUI");
        int showOwnedIndex = Settings::Manager::getInt("show owned", "Game");
        // Match the index with the option (only 0, 1, 2, or 3 are valid). Will default to 0 if invalid.
        if (showOwnedIndex >= 0 && showOwnedIndex <= 3)
            showOwnedComboBox->setCurrentIndex(showOwnedIndex);
        loadSettingBool(stretchBackgroundCheckBox, "stretch menu background", "GUI");
        loadSettingBool(useZoomOnMapCheckBox, "allow zooming", "Map");
        loadSettingBool(graphicHerbalismCheckBox, "graphic herbalism", "Game");
        scalingSpinBox->setValue(Settings::Manager::getFloat("scaling factor", "GUI"));
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
        maximumQuicksavesComboBox->setValue(Settings::Manager::getInt("max quicksaves", "Saves"));

        // Other Settings
        QString screenshotFormatString = QString::fromStdString(Settings::Manager::getString("screenshot format", "General")).toUpper();
        if (screenshotFormatComboBox->findText(screenshotFormatString) == -1)
            screenshotFormatComboBox->addItem(screenshotFormatString);
        screenshotFormatComboBox->setCurrentIndex(screenshotFormatComboBox->findText(screenshotFormatString));

        loadSettingBool(notifyOnSavedScreenshotCheckBox, "notify on saved screenshot", "General");
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
        if (unarmedFactorsStrengthIndex != Settings::Manager::getInt("strength influences hand to hand", "Game"))
            Settings::Manager::setInt("strength influences hand to hand", "Game", unarmedFactorsStrengthIndex);
        saveSettingBool(stealingFromKnockedOutCheckBox, "always allow stealing from knocked out actors", "Game");
        saveSettingBool(enableNavigatorCheckBox, "enable", "Navigator");
        int numPhysicsThreads = physicsThreadsSpinBox->value();
        if (numPhysicsThreads != Settings::Manager::getInt("async num threads", "Physics"))
            Settings::Manager::setInt("async num threads", "Physics", numPhysicsThreads);
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

        const bool distantTerrain = Settings::Manager::getBool("distant terrain", "Terrain");
        const bool objectPaging = Settings::Manager::getBool("object paging", "Terrain");
        const bool wantDistantLand = distantLandCheckBox->checkState();
        if (wantDistantLand != (distantTerrain && objectPaging)) {
            Settings::Manager::setBool("distant terrain", "Terrain", wantDistantLand);
            Settings::Manager::setBool("object paging", "Terrain", wantDistantLand);
        }

        saveSettingBool(activeGridObjectPagingCheckBox, "object paging active grid", "Terrain");
        double viewingDistance = viewingDistanceComboBox->value();
        if (viewingDistance != convertToCells(Settings::Manager::getInt("viewing distance", "Camera")))
        {
            Settings::Manager::setInt("viewing distance", "Camera", convertToUnits(viewingDistance));
        }
        double objectPagingMinSize = objectPagingMinSizeComboBox->value();
        if (objectPagingMinSize != Settings::Manager::getDouble("object paging min size", "Terrain"))
            Settings::Manager::setDouble("object paging min size", "Terrain", objectPagingMinSize);
    }
    
    // Audio
    {
        int audioDeviceIndex = audioDeviceSelectorComboBox->currentIndex();
        if (audioDeviceIndex != 0)
        {
            Settings::Manager::setString("device", "Sound", audioDeviceSelectorComboBox->currentText().toUtf8().constData());
        } 
        else 
        {
            Settings::Manager::setString("device", "Sound", "");
        }
        int hrtfEnabledIndex = enableHRTFComboBox->currentIndex() - 1;
        if (hrtfEnabledIndex != Settings::Manager::getInt("hrtf enable", "Sound"))
        {
            Settings::Manager::setInt("hrtf enable", "Sound", hrtfEnabledIndex);
        }
        int selectedHRTFProfileIndex = hrtfProfileSelectorComboBox->currentIndex();
        if (selectedHRTFProfileIndex != 0)
        {
            Settings::Manager::setString("hrtf", "Sound", hrtfProfileSelectorComboBox->currentText().toUtf8().constData());
        }
        else 
        {
            Settings::Manager::setString("hrtf", "Sound", "");
        }
    }

    // Camera
    {
        saveSettingBool(viewOverShoulderCheckBox, "view over shoulder", "Camera");
        saveSettingBool(autoSwitchShoulderCheckBox, "auto switch shoulder", "Camera");
        saveSettingBool(previewIfStandStillCheckBox, "preview if stand still", "Camera");
        saveSettingBool(deferredPreviewRotationCheckBox, "deferred preview rotation", "Camera");
        saveSettingBool(headBobbingCheckBox, "head bobbing", "Camera");

        osg::Vec2f shoulderOffset = Settings::Manager::getVector2("view over shoulder offset", "Camera");
        if (defaultShoulderComboBox->currentIndex() != (shoulderOffset.x() >= 0 ? 0 : 1))
        {
            if (defaultShoulderComboBox->currentIndex() == 0)
                shoulderOffset.x() = std::abs(shoulderOffset.x());
            else
                shoulderOffset.x() = -std::abs(shoulderOffset.x());
            Settings::Manager::setVector2("view over shoulder offset", "Camera", shoulderOffset);
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
        if (showOwnedCurrentIndex != Settings::Manager::getInt("show owned", "Game"))
            Settings::Manager::setInt("show owned", "Game", showOwnedCurrentIndex);
        saveSettingBool(stretchBackgroundCheckBox, "stretch menu background", "GUI");
        saveSettingBool(useZoomOnMapCheckBox, "allow zooming", "Map");
        saveSettingBool(graphicHerbalismCheckBox, "graphic herbalism", "Game");
        float uiScalingFactor = scalingSpinBox->value();
        if (uiScalingFactor != Settings::Manager::getFloat("scaling factor", "GUI"))
            Settings::Manager::setFloat("scaling factor", "GUI", uiScalingFactor);
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
        if (maximumQuicksaves != Settings::Manager::getInt("max quicksaves", "Saves"))
        {
            Settings::Manager::setInt("max quicksaves", "Saves", maximumQuicksaves);
        }

        // Other Settings
        std::string screenshotFormatString = screenshotFormatComboBox->currentText().toLower().toStdString();
        if (screenshotFormatString != Settings::Manager::getString("screenshot format", "General"))
            Settings::Manager::setString("screenshot format", "General", screenshotFormatString);

        saveSettingBool(notifyOnSavedScreenshotCheckBox, "notify on saved screenshot", "General");
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
    if (Settings::Manager::getBool(setting, group))
        checkbox->setCheckState(Qt::Checked);
}

void Launcher::AdvancedPage::saveSettingBool(QCheckBox *checkbox, const std::string &setting, const std::string &group)
{
    bool cValue = checkbox->checkState();
    if (cValue != Settings::Manager::getBool(setting, group))
        Settings::Manager::setBool(setting, group, cValue);
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

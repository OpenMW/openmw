#include "advancedpage.hpp"

#include <components/config/gamesettings.hpp>
#include <components/config/launchersettings.hpp>
#include <QFileDialog>
#include <QCompleter>
#include <components/contentselector/view/contentselector.hpp>
#include <components/contentselector/model/esmfile.hpp>

Launcher::AdvancedPage::AdvancedPage(Files::ConfigurationManager &cfg,
                                     Config::GameSettings &gameSettings,
                                     Settings::Manager &engineSettings, QWidget *parent)
        : QWidget(parent)
        , mCfgMgr(cfg)
        , mGameSettings(gameSettings)
        , mEngineSettings(engineSettings)
{
    setObjectName ("AdvancedPage");
    setupUi(this);

    loadSettings();
}

void Launcher::AdvancedPage::loadCellsForAutocomplete(QStringList cellNames) {
    // Set up an auto-completer for the "Start default character at" field
    auto *completer = new QCompleter(cellNames);
    completer->setCompletionMode(QCompleter::PopupCompletion);
    completer->setCaseSensitivity(Qt::CaseSensitivity::CaseInsensitive);
    startDefaultCharacterAtField->setCompleter(completer);

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

bool Launcher::AdvancedPage::loadSettings()
{
    // Testing
    bool skipMenu = mGameSettings.value("skip-menu").toInt() == 1;
    if (skipMenu) {
        skipMenuCheckBox->setCheckState(Qt::Checked);
    }
    startDefaultCharacterAtLabel->setEnabled(skipMenu);
    startDefaultCharacterAtField->setEnabled(skipMenu);

    startDefaultCharacterAtField->setText(mGameSettings.value("start"));
    runScriptAfterStartupField->setText(mGameSettings.value("script-run"));

    // Game Settings
    loadSettingBool(canLootDuringDeathAnimationCheckBox, "can loot during death animation", "Game");
    loadSettingBool(followersAttackOnSightCheckBox, "followers attack on sight", "Game");
    loadSettingBool(preventMerchantEquippingCheckBox, "prevent merchant equipping", "Game");
    loadSettingBool(classicReflectedAbsorbSpellsCheckBox, "classic reflected absorb spells behavior", "Game");
    loadSettingBool(rebalanceSoulGemValuesCheckBox, "rebalance soul gem values", "Game");
    loadSettingBool(enchantedWeaponsMagicalCheckBox, "enchanted weapons are magical", "Game");
    loadSettingBool(permanentBarterDispositionChangeCheckBox, "barter disposition change is permanent", "Game");
    int unarmedFactorsStrengthIndex = mEngineSettings.getInt("strength influences hand to hand", "Game");
    if (unarmedFactorsStrengthIndex >= 0 && unarmedFactorsStrengthIndex <= 2)
        unarmedFactorsStrengthComboBox->setCurrentIndex(unarmedFactorsStrengthIndex);
    loadSettingBool(requireAppropriateAmmunitionCheckBox, "only appropriate ammunition bypasses resistance", "Game");
    loadSettingBool(magicItemAnimationsCheckBox, "use magic item animations", "Game");
    loadSettingBool(normaliseRaceSpeedCheckBox, "normalise race speed", "Game");
    connect(animSourcesCheckBox, SIGNAL(toggled(bool)), this, SLOT(slotAnimSourcesToggled(bool)));
    loadSettingBool(animSourcesCheckBox, "use additional anim sources", "Game");
    if (animSourcesCheckBox->checkState())
    {
        loadSettingBool(weaponSheathingCheckBox, "weapon sheathing", "Game");
        loadSettingBool(shieldSheathingCheckBox, "shield sheathing", "Game");
    }
    loadSettingBool(uncappedDamageFatigueCheckBox, "uncapped damage fatigue", "Game");

    // Input Settings
    loadSettingBool(grabCursorCheckBox, "grab cursor", "Input");
    loadSettingBool(toggleSneakCheckBox, "toggle sneak", "Input");

    // Saves Settings
    loadSettingBool(timePlayedCheckbox, "timeplayed", "Saves");
    maximumQuicksavesComboBox->setValue(mEngineSettings.getInt("max quicksaves", "Saves"));

    // User Interface Settings
    loadSettingBool(showEffectDurationCheckBox, "show effect duration", "Game");
    loadSettingBool(showEnchantChanceCheckBox, "show enchant chance", "Game");
    loadSettingBool(showMeleeInfoCheckBox, "show melee info", "Game");
    loadSettingBool(showProjectileDamageCheckBox, "show projectile damage", "Game");
    int showOwnedIndex = mEngineSettings.getInt("show owned", "Game");
    // Match the index with the option (only 0, 1, 2, or 3 are valid). Will default to 0 if invalid.
    if (showOwnedIndex >= 0 && showOwnedIndex <= 3)
        showOwnedComboBox->setCurrentIndex(showOwnedIndex);

    // Other Settings
    QString screenshotFormatString = QString::fromStdString(mEngineSettings.getString("screenshot format", "General")).toUpper();
    if (screenshotFormatComboBox->findText(screenshotFormatString) == -1)
        screenshotFormatComboBox->addItem(screenshotFormatString);
    screenshotFormatComboBox->setCurrentIndex(screenshotFormatComboBox->findText(screenshotFormatString));

    return true;
}

void Launcher::AdvancedPage::saveSettings()
{
    // Ensure we only set the new settings if they changed. This is to avoid cluttering the
    // user settings file (which by definition should only contain settings the user has touched)

    // Testing
    int skipMenu = skipMenuCheckBox->checkState() == Qt::Checked;
    if (skipMenu != mGameSettings.value("skip-menu").toInt())
        mGameSettings.setValue("skip-menu", QString::number(skipMenu));

    QString startCell = startDefaultCharacterAtField->text();
    if (startCell != mGameSettings.value("start")) {
        mGameSettings.setValue("start", startCell);
    }
    QString scriptRun = runScriptAfterStartupField->text();
    if (scriptRun != mGameSettings.value("script-run"))
        mGameSettings.setValue("script-run", scriptRun);

    // Game Settings
    saveSettingBool(canLootDuringDeathAnimationCheckBox, "can loot during death animation", "Game");
    saveSettingBool(followersAttackOnSightCheckBox, "followers attack on sight", "Game");
    saveSettingBool(preventMerchantEquippingCheckBox, "prevent merchant equipping", "Game");
    saveSettingBool(rebalanceSoulGemValuesCheckBox, "rebalance soul gem values", "Game");
    saveSettingBool(classicReflectedAbsorbSpellsCheckBox, "classic reflected absorb spells behavior", "Game");
    saveSettingBool(enchantedWeaponsMagicalCheckBox, "enchanted weapons are magical", "Game");
    saveSettingBool(permanentBarterDispositionChangeCheckBox, "barter disposition change is permanent", "Game");
    int unarmedFactorsStrengthIndex = unarmedFactorsStrengthComboBox->currentIndex();
    if (unarmedFactorsStrengthIndex != mEngineSettings.getInt("strength influences hand to hand", "Game"))
        mEngineSettings.setInt("strength influences hand to hand", "Game", unarmedFactorsStrengthIndex);
    saveSettingBool(requireAppropriateAmmunitionCheckBox, "only appropriate ammunition bypasses resistance", "Game");
    saveSettingBool(magicItemAnimationsCheckBox, "use magic item animations", "Game");
    saveSettingBool(normaliseRaceSpeedCheckBox, "normalise race speed", "Game");
    saveSettingBool(animSourcesCheckBox, "use additional anim sources", "Game");
    saveSettingBool(weaponSheathingCheckBox, "weapon sheathing", "Game");
    saveSettingBool(shieldSheathingCheckBox, "shield sheathing", "Game");
    saveSettingBool(uncappedDamageFatigueCheckBox, "uncapped damage fatigue", "Game");

    // Input Settings
    saveSettingBool(grabCursorCheckBox, "grab cursor", "Input");
    saveSettingBool(toggleSneakCheckBox, "toggle sneak", "Input");

    // Saves Settings
    saveSettingBool(timePlayedCheckbox, "timeplayed", "Saves");
    int maximumQuicksaves = maximumQuicksavesComboBox->value();
    if (maximumQuicksaves != mEngineSettings.getInt("max quicksaves", "Saves")) {
        mEngineSettings.setInt("max quicksaves", "Saves", maximumQuicksaves);
    }

    // User Interface Settings
    saveSettingBool(showEffectDurationCheckBox, "show effect duration", "Game");
    saveSettingBool(showEnchantChanceCheckBox, "show enchant chance", "Game");
    saveSettingBool(showMeleeInfoCheckBox, "show melee info", "Game");
    saveSettingBool(showProjectileDamageCheckBox, "show projectile damage", "Game");
    int showOwnedCurrentIndex = showOwnedComboBox->currentIndex();
    if (showOwnedCurrentIndex != mEngineSettings.getInt("show owned", "Game"))
        mEngineSettings.setInt("show owned", "Game", showOwnedCurrentIndex);

    // Other Settings
    std::string screenshotFormatString = screenshotFormatComboBox->currentText().toLower().toStdString();
    if (screenshotFormatString != mEngineSettings.getString("screenshot format", "General"))
        mEngineSettings.setString("screenshot format", "General", screenshotFormatString);
}

void Launcher::AdvancedPage::loadSettingBool(QCheckBox *checkbox, const std::string &setting, const std::string &group) {
    if (mEngineSettings.getBool(setting, group))
        checkbox->setCheckState(Qt::Checked);
}

void Launcher::AdvancedPage::saveSettingBool(QCheckBox *checkbox, const std::string &setting, const std::string &group) {
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

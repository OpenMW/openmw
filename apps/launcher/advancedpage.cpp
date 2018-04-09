#include "advancedpage.hpp"

#include <components/files/configurationmanager.hpp>

Launcher::AdvancedPage::AdvancedPage(Files::ConfigurationManager &cfg, Settings::Manager &engineSettings, QWidget *parent)
        : QWidget(parent)
        , mCfgMgr(cfg)
        , mEngineSettings(engineSettings)
{
    setObjectName ("AdvancedPage");
    setupUi(this);

    loadSettings();
}

bool Launcher::AdvancedPage::loadSettings()
{
    // Game Settings
    loadSettingBool(canLootDuringDeathAnimationCheckBox, "can loot during death animation", "Game");
    loadSettingBool(followersAttackOnSightCheckBox, "followers attack on sight", "Game");
    loadSettingBool(preventMerchantEquippingCheckBox, "prevent merchant equipping", "Game");
    loadSettingBool(showEffectDurationCheckBox, "show effect duration", "Game");
    loadSettingBool(showEnchantChanceCheckBox, "show enchant chance", "Game");
    loadSettingBool(showMeleeInfoCheckBox, "show melee info", "Game");
    loadSettingBool(showProjectileDamageCheckBox, "show projectile damage", "Game");

    // Expected values are (0, 1, 2, 3)
    int showOwnedIndex = mEngineSettings.getInt("show owned", "Game");
    // Match the index with the option. Will default to 0 if invalid.
    if (showOwnedIndex >= 0 && showOwnedIndex <= 3)
        showOwnedComboBox->setCurrentIndex(showOwnedIndex);

    // Input Settings
    loadSettingBool(allowThirdPersonZoomCheckBox, "allow third person zoom", "Input");
    loadSettingBool(grabCursorCheckBox, "grab cursor", "Input");
    loadSettingBool(toggleSneakCheckBox, "toggle sneak", "Input");

    // Saves Settings
    loadSettingBool(timePlayedCheckbox, "timeplayed", "Saves");
    maximumQuicksavesComboBox->setValue(mEngineSettings.getInt("max quicksaves", "Saves"));

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

    // Game Settings
    saveSettingBool(canLootDuringDeathAnimationCheckBox, "can loot during death animation", "Game");
    saveSettingBool(followersAttackOnSightCheckBox, "followers attack on sight", "Game");
    saveSettingBool(preventMerchantEquippingCheckBox, "prevent merchant equipping", "Game");
    saveSettingBool(showEffectDurationCheckBox, "show effect duration", "Game");
    saveSettingBool(showEnchantChanceCheckBox, "show enchant chance", "Game");
    saveSettingBool(showMeleeInfoCheckBox, "show melee info", "Game");
    saveSettingBool(showProjectileDamageCheckBox, "show projectile damage", "Game");

    int showOwnedCurrentIndex = showOwnedComboBox->currentIndex();
    if (showOwnedCurrentIndex != mEngineSettings.getInt("show owned", "Game"))
        mEngineSettings.setInt("show owned", "Game", showOwnedCurrentIndex);

    // Input Settings
    saveSettingBool(allowThirdPersonZoomCheckBox, "allow third person zoom", "Input");
    saveSettingBool(grabCursorCheckBox, "grab cursor", "Input");
    saveSettingBool(toggleSneakCheckBox, "toggle sneak", "Input");

    // Saves Settings
    saveSettingBool(timePlayedCheckbox, "timeplayed", "Saves");
    int maximumQuicksaves = maximumQuicksavesComboBox->value();
    if (maximumQuicksaves != mEngineSettings.getInt("max quicksaves", "Saves")) {
        mEngineSettings.setInt("max quicksaves", "Saves", maximumQuicksaves);
    }

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
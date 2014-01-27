#include "settingspage.hpp"

Launcher::SettingsPage::SettingsPage(QWidget *parent) : QWidget(parent)
{
    setupUi(this);

    QStringList languages;
    languages << "English"
              << "French"
              << "German"
              << "Italian"
              << "Polish"
              << "Russian"
              << "Spanish";

    languageComboBox->addItems(languages);
}


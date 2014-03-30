#include "settingspage.hpp"

#include <components/process/processinvoker.hpp>

#include <QDebug>

using namespace Process;

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

void Launcher::SettingsPage::on_wizardButton_clicked()
{
    if (!ProcessInvoker::startProcess(QLatin1String("openmw-wizard"), true))
       qDebug() << "an error occurred";

}

void Launcher::SettingsPage::on_importerButton_clicked()
{

}

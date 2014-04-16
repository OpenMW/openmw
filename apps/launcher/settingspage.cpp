#include "settingspage.hpp"

#include <QMessageBox>
#include <QDebug>

#include <components/config/gamesettings.hpp>
#include <components/config/launchersettings.hpp>

#include "utils/textinputdialog.hpp"

using namespace Process;

Launcher::SettingsPage::SettingsPage(Config::GameSettings &gameSettings,
                                     Config::LauncherSettings &launcherSettings, MainDialog *parent) :
    mGameSettings(gameSettings),
    mLauncherSettings(launcherSettings),
    QWidget(parent),
    mMain(parent)
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

    mWizardInvoker = new ProcessInvoker(this);
    mImporterInvoker = new ProcessInvoker(this);

    connect(mWizardInvoker->getProcess(), SIGNAL(started()),
            this, SLOT(wizardStarted()));

    connect(mWizardInvoker->getProcess(), SIGNAL(finished(int,QProcess::ExitStatus)),
            this, SLOT(wizardFinished(int,QProcess::ExitStatus)));

    connect(mImporterInvoker->getProcess(), SIGNAL(started()),
            this, SLOT(importerStarted()));

    connect(mImporterInvoker->getProcess(), SIGNAL(finished(int,QProcess::ExitStatus)),
            this, SLOT(importerFinished(int,QProcess::ExitStatus)));

    mProfileDialog = new TextInputDialog(tr("New Profile"), tr("Profile name:"), this);

    connect(mProfileDialog->lineEdit(), SIGNAL(textChanged(QString)),
            this, SLOT(updateOkButton(QString)));

//    // Detect Morrowind configuration files
//    foreach (const QString &path, mGameSettings.getDataDirs()) {
//        QDir dir(path);
//        dir.setPath(dir.canonicalPath()); // Resolve symlinks

//        if (dir.exists(QString("Morrowind.ini")))
//            iniPaths.append(dir.absoluteFilePath(QString("Morrowind.ini")));
//        else
//        {
//            if (!dir.cdUp())
//                continue; // Cannot move from Data Files

//            if (dir.exists(QString("Morrowind.ini")))
//                iniPaths.append(dir.absoluteFilePath(QString("Morrowind.ini")));
//        }
//    }
}

void Launcher::SettingsPage::on_wizardButton_clicked()
{
    if (!mWizardInvoker->startProcess(QLatin1String("openmw-wizard"), false))
        return;
}

void Launcher::SettingsPage::on_importerButton_clicked()
{
    if (!mImporterInvoker->startProcess(QLatin1String("mwiniimport"), false))
        return;
}

void Launcher::SettingsPage::wizardStarted()
{
    qDebug() << "wizard started!";
    wizardButton->setEnabled(false);
}

void Launcher::SettingsPage::wizardFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    qDebug() << "wizard finished!";
    if (exitCode != 0 || exitStatus == QProcess::CrashExit)
        return;

    mMain->writeSettings();
    mMain->reloadSettings();
    wizardButton->setEnabled(true);
}

void Launcher::SettingsPage::importerStarted()
{
    qDebug() << "importer started!";
    importerButton->setEnabled(false);
}

void Launcher::SettingsPage::importerFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    qDebug() << "importer finished!";
    if (exitCode != 0 || exitStatus == QProcess::CrashExit)
        return;

    mMain->writeSettings();
    mMain->reloadSettings();


    if (addonsCheckBox->isChecked()) {

        if (mProfileDialog->exec() == QDialog::Accepted) {
            QString profile = mProfileDialog->lineEdit()->text();
            qDebug() << profile;
        }
    }

    importerButton->setEnabled(true);
}

void Launcher::SettingsPage::updateOkButton(const QString &text)
{
    // We do this here because we need the profiles combobox text
    if (text.isEmpty()) {
         mProfileDialog->setOkButtonEnabled(false);
         return;
    }


//    (profilesComboBox->findText(text) == -1)
//            ? mNewProfileDialog->setOkButtonEnabled(true)
//            : mNewProfileDialog->setOkButtonEnabled(false);
}

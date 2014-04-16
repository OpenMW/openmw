#include "settingspage.hpp"

#include <QFileDialog>
#include <QMessageBox>
#include <QDebug>
#include <QDir>

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
    languages << QLatin1String("English")
              << QLatin1String("French")
              << QLatin1String("German")
              << QLatin1String("Italian")
              << QLatin1String("Polish")
              << QLatin1String("Russian")
              << QLatin1String("Spanish");

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

    // Detect Morrowind configuration files
    QStringList iniPaths;

    foreach (const QString &path, mGameSettings.getDataDirs()) {
        QDir dir(path);
        dir.setPath(dir.canonicalPath()); // Resolve symlinks

        if (dir.exists(QString("Morrowind.ini")))
            iniPaths.append(dir.absoluteFilePath(QString("Morrowind.ini")));
        else
        {
            if (!dir.cdUp())
                continue; // Cannot move from Data Files

            if (dir.exists(QString("Morrowind.ini")))
                iniPaths.append(dir.absoluteFilePath(QString("Morrowind.ini")));
        }
    }

    if (!iniPaths.isEmpty()) {
        settingsComboBox->addItems(iniPaths);
        importerButton->setEnabled(true);
    } else {
        importerButton->setEnabled(false);
    }
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

void Launcher::SettingsPage::on_browseButton_clicked()
{
    QString iniFile = QFileDialog::getOpenFileName(
                this,
                QObject::tr("Select configuration file"),
                QDir::currentPath(),
                QString(tr("Morrowind configuration file (*.ini)")));


    if (iniFile.isEmpty())
        return;

    QFileInfo info(iniFile);

    if (!info.exists() || !info.isReadable())
        return;

    const QString path(QDir::toNativeSeparators(info.absoluteFilePath()));

    if (settingsComboBox->findText(path) == -1) {
        settingsComboBox->addItem(path);
        settingsComboBox->setCurrentIndex(settingsComboBox->findText(path));
        importerButton->setEnabled(true);
    }
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

    // Import selected data files from openmw.cfg
    if (addonsCheckBox->isChecked())
    {
        if (mProfileDialog->exec() == QDialog::Accepted)
        {
            const QString profile(mProfileDialog->lineEdit()->text());
            const QStringList files(mGameSettings.values(QLatin1String("content")));

            // Doesn't quite work right now
            mLauncherSettings.setValue(QLatin1String("Profiles/currentprofile"), profile);

            foreach (const QString &file, files) {
                mLauncherSettings.setMultiValue(QLatin1String("Profiles/") + profile + QLatin1String("/content"), file);
            }

            mGameSettings.remove(QLatin1String("content"));
        }
    }

    mMain->writeSettings();
    mMain->reloadSettings();
    importerButton->setEnabled(true);
}

void Launcher::SettingsPage::updateOkButton(const QString &text)
{
    // We do this here because we need the profiles combobox text
    if (text.isEmpty()) {
         mProfileDialog->setOkButtonEnabled(false);
         return;
    }

    const QStringList profiles(mLauncherSettings.subKeys(QString("Profiles/")));

    (profiles.contains(text))
            ? mProfileDialog->setOkButtonEnabled(false)
            : mProfileDialog->setOkButtonEnabled(true);
}

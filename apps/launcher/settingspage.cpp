#include "settingspage.hpp"

#include <QFileDialog>
#include <QMessageBox>
#include <QDebug>
#include <QDir>

#include <components/files/configurationmanager.hpp>

#include <components/config/gamesettings.hpp>
#include <components/config/launchersettings.hpp>

#include "utils/textinputdialog.hpp"
#include "datafilespage.hpp"

using namespace Process;

Launcher::SettingsPage::SettingsPage(Files::ConfigurationManager &cfg,
                                     Config::GameSettings &gameSettings,
                                     Config::LauncherSettings &launcherSettings, MainDialog *parent)
    : QWidget(parent)
    , mCfgMgr(cfg)
    , mGameSettings(gameSettings)
    , mLauncherSettings(launcherSettings)
    , mMain(parent)
{
    setupUi(this);

    QStringList languages;
    languages << tr("English")
              << tr("French")
              << tr("German")
              << tr("Italian")
              << tr("Polish")
              << tr("Russian")
              << tr("Spanish");

    languageComboBox->addItems(languages);

    mWizardInvoker = new ProcessInvoker();
    mImporterInvoker = new ProcessInvoker();
    resetProgressBar();

    connect(mWizardInvoker->getProcess(), SIGNAL(started()),
            this, SLOT(wizardStarted()));

    connect(mWizardInvoker->getProcess(), SIGNAL(finished(int,QProcess::ExitStatus)),
            this, SLOT(wizardFinished(int,QProcess::ExitStatus)));

    connect(mImporterInvoker->getProcess(), SIGNAL(started()),
            this, SLOT(importerStarted()));

    connect(mImporterInvoker->getProcess(), SIGNAL(finished(int,QProcess::ExitStatus)),
            this, SLOT(importerFinished(int,QProcess::ExitStatus)));

    mProfileDialog = new TextInputDialog(tr("New Content List"), tr("Content List name:"), this);

    connect(mProfileDialog->lineEdit(), SIGNAL(textChanged(QString)),
            this, SLOT(updateOkButton(QString)));

    // Detect Morrowind configuration files
    QStringList iniPaths;

    for (const QString &path : mGameSettings.getDataDirs())
    {
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

    loadSettings();
}

Launcher::SettingsPage::~SettingsPage()
{
    delete mWizardInvoker;
    delete mImporterInvoker;
}

void Launcher::SettingsPage::on_wizardButton_clicked()
{
    mMain->writeSettings();

    if (!mWizardInvoker->startProcess(QLatin1String("openmw-wizard"), false))
        return;
}

void Launcher::SettingsPage::on_importerButton_clicked()
{
    mMain->writeSettings();

    // Create the file if it doesn't already exist, else the importer will fail
    QString path(QString::fromUtf8(mCfgMgr.getUserConfigPath().string().c_str()));
    path.append(QLatin1String("openmw.cfg"));
    QFile file(path);

    if (!file.exists()) {
        if (!file.open(QIODevice::ReadWrite)) {
            // File cannot be created
            QMessageBox msgBox;
            msgBox.setWindowTitle(tr("Error writing OpenMW configuration file"));
            msgBox.setIcon(QMessageBox::Critical);
            msgBox.setStandardButtons(QMessageBox::Ok);
            msgBox.setText(tr("<html><head/><body><p><b>Could not open or create %1 for writing </b></p> \
                              <p>Please make sure you have the right permissions \
                              and try again.</p></body></html>").arg(file.fileName()));
            msgBox.exec();
            return;
        }

        file.close();
    }

    // Construct the arguments to run the importer
    QStringList arguments;

    if (addonsCheckBox->isChecked())
        arguments.append(QString("--game-files"));

    arguments.append(QString("--encoding"));
    arguments.append(mGameSettings.value(QString("encoding"), QString("win1252")));
    arguments.append(QString("--ini"));
    arguments.append(settingsComboBox->currentText());
    arguments.append(QString("--cfg"));
    arguments.append(path);

    qDebug() << "arguments " << arguments;

    // start the progress bar as a "bouncing ball"
    progressBar->setMaximum(0);
    progressBar->setValue(0);
    if (!mImporterInvoker->startProcess(QLatin1String("openmw-iniimporter"), arguments, false))
    {
        resetProgressBar();
    }
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
    mMain->hide(); // Hide the launcher

    wizardButton->setEnabled(false);
}

void Launcher::SettingsPage::wizardFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    if (exitCode != 0 || exitStatus == QProcess::CrashExit)
        return qApp->quit();

    mMain->reloadSettings();
    wizardButton->setEnabled(true);

    mMain->show(); // Show the launcher again
}

void Launcher::SettingsPage::importerStarted()
{
    importerButton->setEnabled(false);
}

void Launcher::SettingsPage::importerFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    if (exitCode != 0 || exitStatus == QProcess::CrashExit)
    {
        resetProgressBar();

        QMessageBox msgBox;
        msgBox.setWindowTitle(tr("Importer finished"));
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.setIcon(QMessageBox::Warning);
        msgBox.setText(tr("Failed to import settings from INI file."));
        msgBox.exec();
    }
    else
    {
        // indicate progress finished
        progressBar->setMaximum(1);
        progressBar->setValue(1);

        // Importer may have changed settings, so refresh
        mMain->reloadSettings();
    }

    importerButton->setEnabled(true);
}

void Launcher::SettingsPage::resetProgressBar()
{
    // set progress bar to 0 %
    progressBar->reset();
}

void Launcher::SettingsPage::updateOkButton(const QString &text)
{
    // We do this here because we need to access the profiles
    if (text.isEmpty()) {
         mProfileDialog->setOkButtonEnabled(false);
         return;
    }

    const QStringList profiles(mLauncherSettings.getContentLists());

    (profiles.contains(text))
            ? mProfileDialog->setOkButtonEnabled(false)
            : mProfileDialog->setOkButtonEnabled(true);
}

void Launcher::SettingsPage::saveSettings()
{
    QString language(languageComboBox->currentText());

    mLauncherSettings.setValue(QLatin1String("Settings/language"), language);

    if (language == QLatin1String("Polish")) {
        mGameSettings.setValue(QLatin1String("encoding"), QLatin1String("win1250"));
    } else if (language == QLatin1String("Russian")) {
        mGameSettings.setValue(QLatin1String("encoding"), QLatin1String("win1251"));
    }  else {
        mGameSettings.setValue(QLatin1String("encoding"), QLatin1String("win1252"));
    }
}

bool Launcher::SettingsPage::loadSettings()
{
    QString language(mLauncherSettings.value(QLatin1String("Settings/language")));

    int index = languageComboBox->findText(language);

    if (index != -1)
        languageComboBox->setCurrentIndex(index);

    return true;
}

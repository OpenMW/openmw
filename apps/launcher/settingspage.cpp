#include "settingspage.hpp"

#include <QFileDialog>
#include <QMessageBox>
#include <QDebug>
#include <QDir>
#include <QTimer>

#include <components/files/configurationmanager.hpp>

#include <components/config/gamesettings.hpp>
#include <components/config/launchersettings.hpp>

#include "utils/textinputdialog.hpp"
#include "datafilespage.hpp"

using namespace Process;

Launcher::SettingsPage::SettingsPage(Files::ConfigurationManager &cfg,
                                     Config::GameSettings &gameSettings,
                                     Config::LauncherSettings &launcherSettings, MainDialog *parent)
    : mCfgMgr(cfg)
    , mGameSettings(gameSettings)
    , mLauncherSettings(launcherSettings)
    , QWidget(parent)
    , mMain(parent)
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

    mWizardInvoker = new ProcessInvoker();
    mImporterInvoker = new ProcessInvoker();
    mTimer = new QTimer(this);
    progressBar->setValue(0);

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

    connect(mTimer, SIGNAL(timeout()), this, SLOT(onTimer()));

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

    loadSettings();
}

Launcher::SettingsPage::~SettingsPage()
{
    delete mWizardInvoker;
    delete mImporterInvoker;
}

void Launcher::SettingsPage::on_wizardButton_clicked()
{
    saveSettings();

    if (!mWizardInvoker->startProcess(QLatin1String("openmw-wizard"), false))
        return;
}

void Launcher::SettingsPage::on_importerButton_clicked()
{
    saveSettings();

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

    if (!mImporterInvoker->startProcess(QLatin1String("openmw-iniimporter"), arguments, false))
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
        QMessageBox msgBox;
        msgBox.setWindowTitle(tr("Importer finished"));
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.setIcon(QMessageBox::Warning);
        msgBox.setText(tr("Failed to import settings from INI file."));
        msgBox.exec();
    }
    else
    {
        simulateProgress();
    }
}

void Launcher::SettingsPage::reloadSettings()
{
    // Importer may have changed settings, so refresh
    mMain->reloadSettings();

    // Import selected data files from openmw.cfg
    if (addonsCheckBox->isChecked())
    {
        // Because we've reloaded settings, the current content list matches content in OpenMW.cfg
        QString oldContentListName = mLauncherSettings.getCurrentContentListName();
        if (mProfileDialog->exec() == QDialog::Accepted)
        {
            // remove the current content list to prevent duplication
            //... except, not allowed to delete the Default content list
            if (oldContentListName.compare(DataFilesPage::mDefaultContentListName) != 0)
            {
                mLauncherSettings.removeContentList(oldContentListName);
            }

            const QString newContentListName(mProfileDialog->lineEdit()->text());
            const QStringList files(mGameSettings.getContentList());
            mLauncherSettings.setCurrentContentListName(newContentListName);
            mLauncherSettings.setContentList(newContentListName, files);

            // Make DataFiles Page load the new content list.
            mMain->reloadSettings();
        }
    }

    importerButton->setEnabled(true);
}

// Normally, ini import is so fast user won't notice it
// So, we make a progress bar move so user can see something happened.
void Launcher::SettingsPage::simulateProgress()
{
    // update progress bar 5 times second
    const int progressUpdateInterval = 200;

    progressBar->setValue(0);
    mTimer->start(progressUpdateInterval);
}

void Launcher::SettingsPage::onTimer()
{
    int val = progressBar->value();
    ++val;
    progressBar->setValue(val);
    if (progressBar->maximum() <= val)
    {
        mTimer->stop();
        reloadSettings();
    }
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

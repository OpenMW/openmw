#include "importpage.hpp"

#include <QDebug>
#include <QDir>
#include <QFileDialog>
#include <QMessageBox>

#include <components/files/configurationmanager.hpp>
#include <components/files/qtconversion.hpp>

using namespace Process;

Launcher::ImportPage::ImportPage(const Files::ConfigurationManager& cfg, Config::GameSettings& gameSettings,
    Config::LauncherSettings& launcherSettings, MainDialog* parent)
    : QWidget(parent)
    , mCfgMgr(cfg)
    , mGameSettings(gameSettings)
    , mLauncherSettings(launcherSettings)
    , mMain(parent)
{
    setupUi(this);

    mWizardInvoker = new ProcessInvoker();
    mImporterInvoker = new ProcessInvoker();
    resetProgressBar();

    connect(mWizardInvoker->getProcess(), &QProcess::started, this, &ImportPage::wizardStarted);

    connect(mWizardInvoker->getProcess(), qOverload<int, QProcess::ExitStatus>(&QProcess::finished), this,
        &ImportPage::wizardFinished);

    connect(mImporterInvoker->getProcess(), &QProcess::started, this, &ImportPage::importerStarted);

    connect(mImporterInvoker->getProcess(), qOverload<int, QProcess::ExitStatus>(&QProcess::finished), this,
        &ImportPage::importerFinished);

    // Detect Morrowind configuration files
    QStringList iniPaths;

    for (const auto& path : mGameSettings.getDataDirs())
    {
        QDir dir(path.value);
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

    if (!iniPaths.isEmpty())
    {
        settingsComboBox->addItems(iniPaths);
        importerButton->setEnabled(true);
    }
    else
    {
        importerButton->setEnabled(false);
    }

    loadSettings();
}

Launcher::ImportPage::~ImportPage()
{
    delete mWizardInvoker;
    delete mImporterInvoker;
}

void Launcher::ImportPage::on_wizardButton_clicked()
{
    mMain->writeSettings();

    if (!mWizardInvoker->startProcess(QLatin1String("openmw-wizard"), false))
        return;
}

void Launcher::ImportPage::on_importerButton_clicked()
{
    mMain->writeSettings();

    // Create the file if it doesn't already exist, else the importer will fail
    auto path = mCfgMgr.getUserConfigPath();
    path /= "openmw.cfg";
    QFile file(path);

    if (!file.exists())
    {
        if (!file.open(QIODevice::ReadWrite))
        {
            // File cannot be created
            QMessageBox msgBox;
            msgBox.setWindowTitle(tr("Error writing OpenMW configuration file"));
            msgBox.setIcon(QMessageBox::Critical);
            msgBox.setStandardButtons(QMessageBox::Ok);
            msgBox.setText(
                tr("<html><head/><body><p><b>Could not open or create %1 for writing </b></p>"
                   "<p>Please make sure you have the right permissions "
                   "and try again.</p></body></html>")
                    .arg(file.fileName()));
            msgBox.exec();
            return;
        }

        file.close();
    }

    // Construct the arguments to run the importer
    QStringList arguments;

    if (addonsCheckBox->isChecked())
        arguments.append(QString("--game-files"));

    if (fontsCheckBox->isChecked())
        arguments.append(QString("--fonts"));

    arguments.append(QString("--encoding"));
    arguments.append(mGameSettings.value(QString("encoding"), { "win1252" }).value);
    arguments.append(QString("--ini"));
    arguments.append(settingsComboBox->currentText());
    arguments.append(QString("--cfg"));
    arguments.append(Files::pathToQString(path));

    qDebug() << "arguments " << arguments;

    // start the progress bar as a "bouncing ball"
    progressBar->setMaximum(0);
    progressBar->setValue(0);
    if (!mImporterInvoker->startProcess(QLatin1String("openmw-iniimporter"), arguments, false))
    {
        resetProgressBar();
    }
}

void Launcher::ImportPage::on_browseButton_clicked()
{
    QString iniFile = QFileDialog::getOpenFileName(this, QObject::tr("Select configuration file"), QDir::currentPath(),
        QString(tr("Morrowind configuration file (*.ini)")));

    if (iniFile.isEmpty())
        return;

    QFileInfo info(iniFile);

    if (!info.exists() || !info.isReadable())
        return;

    const QString path(QDir::toNativeSeparators(info.absoluteFilePath()));

    if (settingsComboBox->findText(path) == -1)
    {
        settingsComboBox->addItem(path);
        settingsComboBox->setCurrentIndex(settingsComboBox->findText(path));
        importerButton->setEnabled(true);
    }
}

void Launcher::ImportPage::wizardStarted()
{
    mMain->hide(); // Hide the launcher

    wizardButton->setEnabled(false);
}

void Launcher::ImportPage::wizardFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    if (exitCode != 0 || exitStatus == QProcess::CrashExit)
        return qApp->quit();

    mMain->reloadSettings();
    wizardButton->setEnabled(true);

    mMain->show(); // Show the launcher again
}

void Launcher::ImportPage::importerStarted()
{
    importerButton->setEnabled(false);
}

void Launcher::ImportPage::importerFinished(int exitCode, QProcess::ExitStatus exitStatus)
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

void Launcher::ImportPage::resetProgressBar()
{
    // set progress bar to 0 %
    progressBar->reset();
}

void Launcher::ImportPage::saveSettings()
{
    mLauncherSettings.setImportContentSetup(addonsCheckBox->isChecked());
    mLauncherSettings.setImportFontSetup(fontsCheckBox->isChecked());
}

bool Launcher::ImportPage::loadSettings()
{
    addonsCheckBox->setChecked(mLauncherSettings.getImportContentSetup());
    fontsCheckBox->setChecked(mLauncherSettings.getImportFontSetup());
    return true;
}

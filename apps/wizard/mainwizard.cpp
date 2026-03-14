#include "mainwizard.hpp"

#include <algorithm>

#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QMessageBox>
#include <QProcess>

#include <components/debug/debugging.hpp>
#include <components/files/qtconfigpath.hpp>
#include <components/files/qtconversion.hpp>
#include <components/misc/utf8qtextstream.hpp>
#include <components/process/processinvoker.hpp>

#include "componentselectionpage.hpp"
#include "conclusionpage.hpp"
#include "existinginstallationpage.hpp"
#include "importpage.hpp"
#include "installationtargetpage.hpp"
#include "intropage.hpp"
#include "languageselectionpage.hpp"
#include "methodselectionpage.hpp"

#ifdef OPENMW_USE_UNSHIELD
#include "installationpage.hpp"
#endif

Wizard::MainWizard::MainWizard(Files::ConfigurationManager&& cfgMgr, QWidget* parent)
    : QWizard(parent)
    , mCfgMgr(cfgMgr)
    , mImporterInvoker(new Process::ProcessInvoker())
    , mGameSettings(mCfgMgr)
{
#ifndef Q_OS_MAC
    setWizardStyle(QWizard::ModernStyle);
#else
    setWizardStyle(QWizard::ClassicStyle);
#endif

    setWindowTitle(tr("OpenMW Wizard"));
    setWindowIcon(QIcon(QLatin1String(":/images/openmw-wizard.png")));
    setMinimumWidth(550);

    // Set the property for comboboxes to the text instead of index
    setDefaultProperty("QComboBox", "currentText", "currentIndexChanged");

    connect(mImporterInvoker->getProcess(), qOverload<int, QProcess::ExitStatus>(&QProcess::finished), this,
        &MainWizard::importerFinished);

    Log(Debug::Info) << "Started OpenMW Wizard on " << QDateTime::currentDateTime().toString().toUtf8().constData();

    std::filesystem::create_directories(mCfgMgr.getUserConfigPath());

    const QString userPath(Files::pathToQString(mCfgMgr.getUserConfigPath()));
    if (!QDir(userPath).exists())
    {
        const QString message = tr(
            "<html><head/><body><p><b>Could not create %1</b></p>"
            "<p>Please make sure you have the right permissions and try again.</p></body></html>");
        showCriticalError(tr("Error creating OpenMW configuration directory"), message.arg(userPath));
        // TODO: consider quitting instantly if this or config file opening fails
    }

    std::filesystem::create_directories(mCfgMgr.getUserDataPath());

    setupGameSettings();
    setupLauncherSettings();
    setupInstallations();
    setupPages();

    for (const std::filesystem::path& installationPath : mCfgMgr.getInstallPaths())
    {
        addInstallation(Files::pathToQString(installationPath / "Data Files"));
    }
}

Wizard::MainWizard::~MainWizard() = default;

void Wizard::MainWizard::setupGameSettings()
{
    const QString title = tr("Error opening OpenMW configuration file");
    const QString message = tr(
        "<html><head/><body><p><b>Could not open %1 for reading</b></p>"
        "<p>Please make sure you have the right permissions and try again.</p></body></html>");

    // Load the user config file first, separately
    // So we can write it properly, uncontaminated
    const QString path(Files::getUserConfigPathQString(mCfgMgr));
    QFile file(path);

    qDebug() << "Loading config file:" << path.toUtf8().constData();

    if (file.exists())
    {
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        {
            showCriticalError(title, message.arg(file.fileName()));
            return;
        }
        QTextStream stream(&file);
        Misc::ensureUtf8Encoding(stream);

        mGameSettings.readUserFile(stream, QFileInfo(path).dir().path());
        file.close();
    }

    // Now the rest
    const QStringList paths = Files::getActiveConfigPathsQString(mCfgMgr);

    for (const QString& path2 : paths)
    {
        qDebug() << "Loading config file:" << path2.toUtf8().constData();

        file.setFileName(path2);
        if (file.exists())
        {
            if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
            {
                showCriticalError(title, message.arg(file.fileName()));
                return;
            }
            QTextStream stream(&file);
            Misc::ensureUtf8Encoding(stream);

            mGameSettings.readFile(stream, QFileInfo(path2).dir().path());
            file.close();
        }
    }
}

void Wizard::MainWizard::setupLauncherSettings()
{
    const std::filesystem::path configPath = mCfgMgr.getUserConfigPath();
    const QString path(Files::pathToQString(configPath / Config::LauncherSettings::sLauncherConfigFileName));

    QFile file(path);

    qDebug() << "Loading config file:" << path.toUtf8().constData();

    if (file.exists())
    {
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        {
            const QString title = tr("Error opening OpenMW configuration file");
            const QString message = tr(
                "<html><head/><body><p><b>Could not open %1 for reading</b></p>"
                "<p>Please make sure you have the right permissions "
                "and try again.</p></body></html>");
            showCriticalError(title, message.arg(file.fileName()));
            return;
        }
        QTextStream stream(&file);
        Misc::ensureUtf8Encoding(stream);

        mLauncherSettings.readFile(stream);
    }
}

void Wizard::MainWizard::setupInstallations()
{
    // Check if the paths actually contain a Morrowind installation
    for (const auto& path : mGameSettings.getDataDirs())
    {
        if (findFiles(QLatin1String("Morrowind"), path.value))
            addInstallation(path.value);
    }
}

void Wizard::MainWizard::runSettingsImporter()
{
    writeSettings();

    const QString path(field(QLatin1String("installation.path")).toString());
    const bool retailDisc(field(QLatin1String("installation.retailDisc")).toBool());

    QFile file(Files::getUserConfigPathQString(mCfgMgr));

    // Construct the arguments to run the importer
    QStringList arguments;

    // Import plugin selection?
    if (retailDisc || field(QLatin1String("installation.import-addons")).toBool())
        arguments.append(QLatin1String("--game-files"));

    arguments.append(QLatin1String("--encoding"));

    // Set encoding
    const QString language(field(QLatin1String("installation.language")).toString());
    if (language == QLatin1String("Polish"))
    {
        arguments.append(QLatin1String("win1250"));
    }
    else if (language == QLatin1String("Russian"))
    {
        arguments.append(QLatin1String("win1251"));
    }
    else
    {
        arguments.append(QLatin1String("win1252"));
    }

    // Import fonts
    if (field(QLatin1String("installation.import-fonts")).toBool())
        arguments.append(QLatin1String("--fonts"));

    // Now the paths
    arguments.append(QLatin1String("--ini"));

    if (retailDisc)
    {
        arguments.append(QDir(path).filePath(QLatin1String("Morrowind.ini")));
    }
    else
    {
        arguments.append(mInstallations[path].iniPath);
    }

    arguments.append(QLatin1String("--cfg"));
    arguments.append(Files::getUserConfigPathQString(mCfgMgr));

    if (!mImporterInvoker->startProcess(QLatin1String("openmw-iniimporter"), arguments, false))
        return qApp->quit();
}

void Wizard::MainWizard::addInstallation(const QString& path)
{
    qDebug() << "add installation in: " << path;
    Installation install;

    install.hasMorrowind = findFiles(QLatin1String("Morrowind"), path);
    install.hasTribunal = findFiles(QLatin1String("Tribunal"), path);
    install.hasBloodmoon = findFiles(QLatin1String("Bloodmoon"), path);

    // Try to autodetect the Morrowind.ini location
    // The installation path is the Data Files directory,
    // so the INI should be located in the parent directory.
    QDir dir(path);
    dir.cdUp();
    const QFile file(dir.filePath(QLatin1String("Morrowind.ini")));
    if (file.exists())
        install.iniPath = file.fileName();

    mInstallations.insert(QDir::toNativeSeparators(path), install);

    // Add it to the openmw.cfg too
    const auto& dataDirs = mGameSettings.getDataDirs();
    if (std::none_of(dataDirs.begin(), dataDirs.end(), [&](const Config::SettingValue& d) { return d.value == path; }))
    {
        mGameSettings.setMultiValue(QLatin1String("data"), { path });
        mGameSettings.addDataDir({ path });
    }
}

void Wizard::MainWizard::setupPages()
{
    setPage(Page_Intro, new IntroPage(this));
    setPage(Page_MethodSelection, new MethodSelectionPage(this));
    setPage(Page_LanguageSelection, new LanguageSelectionPage(this));
    setPage(Page_ExistingInstallation, new ExistingInstallationPage(this));
    setPage(Page_InstallationTarget, new InstallationTargetPage(this, mCfgMgr));
    setPage(Page_ComponentSelection, new ComponentSelectionPage(this));
#ifdef OPENMW_USE_UNSHIELD
    setPage(Page_Installation, new InstallationPage(this, mGameSettings));
#endif
    setPage(Page_Import, new ImportPage(this));
    setPage(Page_Conclusion, new ConclusionPage(this));
    setStartId(Page_Intro);
}

void Wizard::MainWizard::importerFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    if (exitCode != 0 || exitStatus == QProcess::CrashExit)
        return;

    // Re-read the settings
    setupGameSettings();
}

void Wizard::MainWizard::accept()
{
    writeSettings();
    QWizard::accept();
}

void Wizard::MainWizard::reject()
{
    QMessageBox msgBox(this);
    msgBox.setWindowTitle(tr("Quit Wizard"));
    msgBox.setIcon(QMessageBox::Question);
    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    msgBox.setText(tr("Are you sure you want to exit the Wizard?"));

    if (msgBox.exec() == QMessageBox::Yes)
    {
        QWizard::reject();
    }
}

void Wizard::MainWizard::writeSettings()
{
    // Write the encoding and language settings
    const QString language(field(QLatin1String("installation.language")).toString());
    mLauncherSettings.setLanguage(language);

    if (language == QLatin1String("Polish"))
    {
        mGameSettings.setValue(QLatin1String("encoding"), { "win1250" });
    }
    else if (language == QLatin1String("Russian"))
    {
        mGameSettings.setValue(QLatin1String("encoding"), { "win1251" });
    }
    else
    {
        mGameSettings.setValue(QLatin1String("encoding"), { "win1252" });
    }

    // Write the installation path so that openmw can find them
    const QString path(field(QLatin1String("installation.path")).toString());

    // Make sure the installation path is the last data= entry
    mGameSettings.removeDataDir(path);
    mGameSettings.addDataDir({ path });

    // Game settings
    QFile file(Files::getUserConfigPathQString(mCfgMgr));

    const QString writeTitle = tr("Error writing OpenMW configuration file");
    const QString writeMessage = tr(
        "<html><head/><body><p><b>Could not open %1 for writing</b></p>"
        "<p>Please make sure you have the right permissions "
        "and try again.</p></body></html>");

    if (!file.open(QIODevice::ReadWrite | QIODevice::Text | QIODevice::Truncate))
    {
        showCriticalError(writeTitle, writeMessage.arg(file.fileName()), this);
        return;
    }

    QTextStream stream(&file);
    Misc::ensureUtf8Encoding(stream);

    mGameSettings.writeFile(stream);
    file.close();

    // Launcher settings
    file.setFileName(
        Files::pathToQString(mCfgMgr.getUserConfigPath() / Config::LauncherSettings::sLauncherConfigFileName));

    if (!file.open(QIODevice::ReadWrite | QIODevice::Text | QIODevice::Truncate))
    {
        showCriticalError(writeTitle, writeMessage.arg(file.fileName()), this);
        return;
    }

    stream.setDevice(&file);
    Misc::ensureUtf8Encoding(stream);

    mLauncherSettings.writeFile(stream);
    file.close();
}

bool Wizard::MainWizard::findFiles(const QString& name, const QString& path)
{
    const QDir dir(path);

    if (!dir.exists())
        return false;

    const QStringList entries = dir.entryList();
    // TODO: add MIME handling to make sure the files are real
    return entries.contains(name + QLatin1String(".esm"), Qt::CaseInsensitive)
        && entries.contains(name + QLatin1String(".bsa"), Qt::CaseInsensitive);
}

void Wizard::MainWizard::showCriticalError(const QString& title, const QString& message, QWidget* parent)
{
    QMessageBox msgBox(parent);
    msgBox.setWindowTitle(title);
    msgBox.setIcon(QMessageBox::Critical);
    msgBox.setStandardButtons(QMessageBox::Ok);
    msgBox.setText(message);
    connect(&msgBox, &QDialog::finished, qApp, &QApplication::quit, Qt::QueuedConnection);
    msgBox.exec();
}

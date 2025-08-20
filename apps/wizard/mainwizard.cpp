#include "mainwizard.hpp"

#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QMessageBox>
#include <QProcess>

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

#include <algorithm>

using namespace Process;

Wizard::MainWizard::MainWizard(Files::ConfigurationManager&& cfgMgr, QWidget* parent)
    : QWizard(parent)
    , mInstallations()
    , mCfgMgr(cfgMgr)
    , mError(false)
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

    setDefaultProperty("ComponentListWidget", "mCheckedItems", "checkedItemsChanged");

    mImporterInvoker = new ProcessInvoker();

    connect(mImporterInvoker->getProcess(), &QProcess::started, this, &MainWizard::importerStarted);

    connect(mImporterInvoker->getProcess(), qOverload<int, QProcess::ExitStatus>(&QProcess::finished), this,
        &MainWizard::importerFinished);

    mLogError = tr(
        "<html><head/><body><p><b>Could not open %1 for writing</b></p>"
        "<p>Please make sure you have the right permissions "
        "and try again.</p></body></html>");

    std::filesystem::create_directories(mCfgMgr.getUserConfigPath());
    std::filesystem::create_directories(mCfgMgr.getUserDataPath());

    setupLog();
    setupGameSettings();
    setupLauncherSettings();
    setupInstallations();
    setupPages();

    const std::filesystem::path& installationPath = mCfgMgr.getInstallPath();
    if (!installationPath.empty())
    {
        const std::filesystem::path& dataPath = installationPath / "Data Files";
        addInstallation(Files::pathToQString(dataPath));
    }
}

Wizard::MainWizard::~MainWizard()
{
    delete mImporterInvoker;
}

void Wizard::MainWizard::setupLog()
{
    QString logPath(Files::pathToQString(mCfgMgr.getLogPath()));
    logPath.append(QLatin1String("wizard.log"));

    QFile file(logPath);

    if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate))
    {
        QMessageBox msgBox;
        msgBox.setWindowTitle(tr("Error opening Wizard log file"));
        msgBox.setIcon(QMessageBox::Critical);
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.setText(mLogError.arg(file.fileName()));
        connect(&msgBox, &QDialog::finished, qApp, &QApplication::quit, Qt::QueuedConnection);
        msgBox.exec();
        return;
    }

    addLogText(QString("Started OpenMW Wizard on %1").arg(QDateTime::currentDateTime().toString()));

    qDebug() << logPath;
}

void Wizard::MainWizard::addLogText(const QString& text)
{
    QString logPath(Files::pathToQString(mCfgMgr.getLogPath()));
    logPath.append(QLatin1String("wizard.log"));

    QFile file(logPath);

    if (!file.open(QIODevice::ReadWrite | QIODevice::Text))
    {
        QMessageBox msgBox;
        msgBox.setWindowTitle(tr("Error opening Wizard log file"));
        msgBox.setIcon(QMessageBox::Critical);
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.setText(mLogError.arg(file.fileName()));
        connect(&msgBox, &QDialog::finished, qApp, &QApplication::quit, Qt::QueuedConnection);
        msgBox.exec();
        return;
    }

    if (!file.isSequential())
        file.seek(file.size());

    QTextStream out(&file);

    if (!text.isEmpty())
    {
        out << text << "\n";
        out.flush();
    }
}

void Wizard::MainWizard::setupGameSettings()
{
    QString message(
        tr("<html><head/><body><p><b>Could not open %1 for reading</b></p>"
           "<p>Please make sure you have the right permissions "
           "and try again.</p></body></html>"));

    // Load the user config file first, separately
    // So we can write it properly, uncontaminated
    QString path(Files::getUserConfigPathQString(mCfgMgr));
    QFile file(path);

    qDebug() << "Loading config file:" << path.toUtf8().constData();

    if (file.exists())
    {
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        {
            QMessageBox msgBox;
            msgBox.setWindowTitle(tr("Error opening OpenMW configuration file"));
            msgBox.setIcon(QMessageBox::Critical);
            msgBox.setStandardButtons(QMessageBox::Ok);
            msgBox.setText(message.arg(file.fileName()));
            connect(&msgBox, &QDialog::finished, qApp, &QApplication::quit, Qt::QueuedConnection);
            msgBox.exec();
            return;
        }
        QTextStream stream(&file);
        Misc::ensureUtf8Encoding(stream);

        mGameSettings.readUserFile(stream, QFileInfo(path).dir().path());
    }

    file.close();

    // Now the rest
    QStringList paths = Files::getActiveConfigPathsQString(mCfgMgr);

    for (const QString& path2 : paths)
    {
        qDebug() << "Loading config file:" << path2.toUtf8().constData();

        file.setFileName(path2);
        if (file.exists())
        {
            if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
            {
                QMessageBox msgBox;
                msgBox.setWindowTitle(tr("Error opening OpenMW configuration file"));
                msgBox.setIcon(QMessageBox::Critical);
                msgBox.setStandardButtons(QMessageBox::Ok);
                msgBox.setText(message.arg(file.fileName()));
                connect(&msgBox, &QDialog::finished, qApp, &QApplication::quit, Qt::QueuedConnection);
                msgBox.exec();
                return;
            }
            QTextStream stream(&file);
            Misc::ensureUtf8Encoding(stream);

            mGameSettings.readFile(stream, QFileInfo(path2).dir().path());
        }
        file.close();
    }
}

void Wizard::MainWizard::setupLauncherSettings()
{
    QString path(Files::pathToQString(mCfgMgr.getUserConfigPath()));
    path.append(QLatin1String(Config::LauncherSettings::sLauncherConfigFileName));

    QString message(
        tr("<html><head/><body><p><b>Could not open %1 for reading</b></p>"
           "<p>Please make sure you have the right permissions "
           "and try again.</p></body></html>"));

    QFile file(path);

    qDebug() << "Loading config file:" << path.toUtf8().constData();

    if (file.exists())
    {
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        {
            QMessageBox msgBox;
            msgBox.setWindowTitle(tr("Error opening OpenMW configuration file"));
            msgBox.setIcon(QMessageBox::Critical);
            msgBox.setStandardButtons(QMessageBox::Ok);
            msgBox.setText(message.arg(file.fileName()));
            connect(&msgBox, &QDialog::finished, qApp, &QApplication::quit, Qt::QueuedConnection);
            msgBox.exec();
            return;
        }
        QTextStream stream(&file);
        Misc::ensureUtf8Encoding(stream);

        mLauncherSettings.readFile(stream);
    }

    file.close();
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

    QString path(field(QLatin1String("installation.path")).toString());

    QFile file(Files::getUserConfigPathQString(mCfgMgr));

    // Construct the arguments to run the importer
    QStringList arguments;

    // Import plugin selection?
    if (field(QLatin1String("installation.retailDisc")).toBool() == true
        || field(QLatin1String("installation.import-addons")).toBool() == true)
        arguments.append(QLatin1String("--game-files"));

    arguments.append(QLatin1String("--encoding"));

    // Set encoding
    QString language(field(QLatin1String("installation.language")).toString());
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
    if (field(QLatin1String("installation.import-fonts")).toBool() == true)
        arguments.append(QLatin1String("--fonts"));

    // Now the paths
    arguments.append(QLatin1String("--ini"));

    if (field(QLatin1String("installation.retailDisc")).toBool() == true)
    {
        arguments.append(path + QDir::separator() + QLatin1String("Morrowind.ini"));
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
    Installation install; // = new Installation();

    install.hasMorrowind = findFiles(QLatin1String("Morrowind"), path);
    install.hasTribunal = findFiles(QLatin1String("Tribunal"), path);
    install.hasBloodmoon = findFiles(QLatin1String("Bloodmoon"), path);

    // Try to autodetect the Morrowind.ini location
    QDir dir(path);
    QFile file(dir.filePath("Morrowind.ini"));

    // Try the parent directory
    // In normal Morrowind installations that's where Morrowind.ini is
    if (!file.exists())
    {
        dir.cdUp();
        file.setFileName(dir.filePath(QLatin1String("Morrowind.ini")));
    }

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

void Wizard::MainWizard::importerStarted() {}

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
    QMessageBox msgBox;
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
    QString language(field(QLatin1String("installation.language")).toString());
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
    QString path(field(QLatin1String("installation.path")).toString());

    // Make sure the installation path is the last data= entry
    mGameSettings.removeDataDir(path);
    mGameSettings.addDataDir({ path });

    QString userPath(Files::pathToQString(mCfgMgr.getUserConfigPath()));
    QDir dir(userPath);

    if (!dir.exists())
    {
        if (!dir.mkpath(userPath))
        {
            QMessageBox msgBox;
            msgBox.setWindowTitle(tr("Error creating OpenMW configuration directory"));
            msgBox.setIcon(QMessageBox::Critical);
            msgBox.setStandardButtons(QMessageBox::Ok);
            msgBox.setText(
                tr("<html><head/><body><p><b>Could not create %1</b></p>"
                   "<p>Please make sure you have the right permissions "
                   "and try again.</p></body></html>")
                    .arg(userPath));
            connect(&msgBox, &QDialog::finished, qApp, &QApplication::quit, Qt::QueuedConnection);
            msgBox.exec();
            return;
        }
    }

    // Game settings
    QFile file(Files::getUserConfigPathQString(mCfgMgr));

    if (!file.open(QIODevice::ReadWrite | QIODevice::Text | QIODevice::Truncate))
    {
        // File cannot be opened or created
        QMessageBox msgBox;
        msgBox.setWindowTitle(tr("Error writing OpenMW configuration file"));
        msgBox.setIcon(QMessageBox::Critical);
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.setText(
            tr("<html><head/><body><p><b>Could not open %1 for writing</b></p>"
               "<p>Please make sure you have the right permissions "
               "and try again.</p></body></html>")
                .arg(file.fileName()));
        connect(&msgBox, &QDialog::finished, qApp, &QApplication::quit, Qt::QueuedConnection);
        msgBox.exec();
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
        // File cannot be opened or created
        QMessageBox msgBox;
        msgBox.setWindowTitle(tr("Error writing OpenMW configuration file"));
        msgBox.setIcon(QMessageBox::Critical);
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.setText(
            tr("<html><head/><body><p><b>Could not open %1 for writing</b></p>"
               "<p>Please make sure you have the right permissions "
               "and try again.</p></body></html>")
                .arg(file.fileName()));
        connect(&msgBox, &QDialog::finished, qApp, &QApplication::quit, Qt::QueuedConnection);
        msgBox.exec();
        return;
    }

    stream.setDevice(&file);
    Misc::ensureUtf8Encoding(stream);

    mLauncherSettings.writeFile(stream);
    file.close();
}

bool Wizard::MainWizard::findFiles(const QString& name, const QString& path)
{
    QDir dir(path);

    if (!dir.exists())
        return false;

    // TODO: add MIME handling to make sure the files are real
    return (dir.entryList().contains(name + QLatin1String(".esm"), Qt::CaseInsensitive)
        && dir.entryList().contains(name + QLatin1String(".bsa"), Qt::CaseInsensitive));
}

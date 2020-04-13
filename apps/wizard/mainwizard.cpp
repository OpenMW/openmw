#include "mainwizard.hpp"

#include <QDebug>

#include <QTime>
#include <QCloseEvent>
#include <QMessageBox>
#include <QTextCodec>
#include <QDir>

#include "intropage.hpp"
#include "methodselectionpage.hpp"
#include "languageselectionpage.hpp"
#include "existinginstallationpage.hpp"
#include "installationtargetpage.hpp"
#include "componentselectionpage.hpp"
#include "importpage.hpp"
#include "conclusionpage.hpp"

#ifdef OPENMW_USE_UNSHIELD
#include "installationpage.hpp"
#endif

using namespace Process;

Wizard::MainWizard::MainWizard(QWidget *parent) :
    QWizard(parent),
    mInstallations(),
    mError(false),
    mGameSettings(mCfgMgr)
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

    connect(mImporterInvoker->getProcess(), SIGNAL(started()),
            this, SLOT(importerStarted()));

    connect(mImporterInvoker->getProcess(), SIGNAL(finished(int,QProcess::ExitStatus)),
            this, SLOT(importerFinished(int,QProcess::ExitStatus)));

    mLogError = tr("<html><head/><body><p><b>Could not open %1 for writing</b></p> \
                   <p>Please make sure you have the right permissions \
                   and try again.</p></body></html>");

    setupLog();
    setupGameSettings();
    setupLauncherSettings();
    setupInstallations();
    setupPages();

    const boost::filesystem::path& installationPath = mCfgMgr.getInstallPath();
    if (!installationPath.empty())
    {
        const boost::filesystem::path& dataPath = installationPath / "Data Files";
        addInstallation(toQString(dataPath));
    }
}

Wizard::MainWizard::~MainWizard()
{
    delete mImporterInvoker;
}

void Wizard::MainWizard::setupLog()
{
    QString logPath(toQString(mCfgMgr.getLogPath()));
    logPath.append(QLatin1String("wizard.log"));

    QFile file(logPath);

    if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        QMessageBox msgBox;
        msgBox.setWindowTitle(tr("Error opening Wizard log file"));
        msgBox.setIcon(QMessageBox::Critical);
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.setText(mLogError.arg(file.fileName()));
        msgBox.exec();
        return qApp->quit();
    }

    addLogText(QString("Started OpenMW Wizard on %1").arg(QDateTime::currentDateTime().toString()));

    qDebug() << logPath;
}

void Wizard::MainWizard::addLogText(const QString &text)
{
    QString logPath(toQString(mCfgMgr.getLogPath()));
    logPath.append(QLatin1String("wizard.log"));

    QFile file(logPath);

    if (!file.open(QIODevice::ReadWrite | QIODevice::Text)) {
        QMessageBox msgBox;
        msgBox.setWindowTitle(tr("Error opening Wizard log file"));
        msgBox.setIcon(QMessageBox::Critical);
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.setText(mLogError.arg(file.fileName()));
        msgBox.exec();
        return qApp->quit();
    }

    if (!file.isSequential())
        file.seek(file.size());

    QTextStream out(&file);

    if (!text.isEmpty())
        out << text << endl;

//    file.close();
}

void Wizard::MainWizard::setupGameSettings()
{
    QString userPath(toQString(mCfgMgr.getUserConfigPath()));
    QString globalPath(toQString(mCfgMgr.getGlobalPath()));
    QString message(tr("<html><head/><body><p><b>Could not open %1 for reading</b></p> \
                    <p>Please make sure you have the right permissions \
                    and try again.</p></body></html>"));

    // Load the user config file first, separately
    // So we can write it properly, uncontaminated
    QString path(userPath + QLatin1String("openmw.cfg"));
    QFile file(path);

    qDebug() << "Loading config file:" << path.toUtf8().constData();

    if (file.exists()) {
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QMessageBox msgBox;
            msgBox.setWindowTitle(tr("Error opening OpenMW configuration file"));
            msgBox.setIcon(QMessageBox::Critical);
            msgBox.setStandardButtons(QMessageBox::Ok);
            msgBox.setText(message.arg(file.fileName()));
            msgBox.exec();
            return qApp->quit();
        }
        QTextStream stream(&file);
        stream.setCodec(QTextCodec::codecForName("UTF-8"));

        mGameSettings.readUserFile(stream);
    }

    // Now the rest
    QStringList paths;
    paths.append(userPath + QLatin1String("openmw.cfg"));
    paths.append(QLatin1String("openmw.cfg"));
    paths.append(globalPath + QLatin1String("openmw.cfg"));

    for (const QString &path2 : paths)
    {
        qDebug() << "Loading config file:" << path2.toUtf8().constData();

        file.setFileName(path2);
        if (file.exists()) {
            if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                QMessageBox msgBox;
                msgBox.setWindowTitle(tr("Error opening OpenMW configuration file"));
                msgBox.setIcon(QMessageBox::Critical);
                msgBox.setStandardButtons(QMessageBox::Ok);
                msgBox.setText(message.arg(file.fileName()));

                return qApp->quit();
            }
            QTextStream stream(&file);
            stream.setCodec(QTextCodec::codecForName("UTF-8"));

            mGameSettings.readFile(stream);
        }
        file.close();
    }
}

void Wizard::MainWizard::setupLauncherSettings()
{
    QString path(toQString(mCfgMgr.getUserConfigPath()));
    path.append(QLatin1String(Config::LauncherSettings::sLauncherConfigFileName));

    QString message(tr("<html><head/><body><p><b>Could not open %1 for reading</b></p> \
                    <p>Please make sure you have the right permissions \
                    and try again.</p></body></html>"));


    QFile file(path);

    qDebug() << "Loading config file:" << path.toUtf8().constData();

    if (file.exists()) {
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QMessageBox msgBox;
            msgBox.setWindowTitle(tr("Error opening OpenMW configuration file"));
            msgBox.setIcon(QMessageBox::Critical);
            msgBox.setStandardButtons(QMessageBox::Ok);
            msgBox.setText(message.arg(file.fileName()));
            msgBox.exec();
            return qApp->quit();
        }
        QTextStream stream(&file);
        stream.setCodec(QTextCodec::codecForName("UTF-8"));

        mLauncherSettings.readFile(stream);
    }

    file.close();

}

void Wizard::MainWizard::setupInstallations()
{
    // Check if the paths actually contain a Morrowind installation
    for (const QString& path : mGameSettings.getDataDirs())
    {

        if (findFiles(QLatin1String("Morrowind"), path))
            addInstallation(path);
    }
}

void Wizard::MainWizard::runSettingsImporter()
{
    writeSettings();

    QString path(field(QLatin1String("installation.path")).toString());

    QString userPath(toQString(mCfgMgr.getUserConfigPath()));
    QFile file(userPath + QLatin1String("openmw.cfg"));

    // Construct the arguments to run the importer
    QStringList arguments;

    // Import plugin selection?
    if (field(QLatin1String("installation.retailDisc")).toBool() == true
            || field(QLatin1String("installation.import-addons")).toBool() == true)
        arguments.append(QLatin1String("--game-files"));

    arguments.append(QLatin1String("--encoding"));

    // Set encoding
    QString language(field(QLatin1String("installation.language")).toString());

    if (language == QLatin1String("Polish")) {
        arguments.append(QLatin1String("win1250"));
    } else if (language == QLatin1String("Russian")) {
        arguments.append(QLatin1String("win1251"));
    }  else {
        arguments.append(QLatin1String("win1252"));
    }

    // Now the paths
    arguments.append(QLatin1String("--ini"));

    if (field(QLatin1String("installation.retailDisc")).toBool() == true) {
        arguments.append(path + QDir::separator() + QLatin1String("Morrowind.ini"));
    } else {
        arguments.append(mInstallations[path].iniPath);
    }

    arguments.append(QLatin1String("--cfg"));
    arguments.append(userPath + QLatin1String("openmw.cfg"));

    if (!mImporterInvoker->startProcess(QLatin1String("openmw-iniimporter"), arguments, false))
        return qApp->quit();
}

void Wizard::MainWizard::addInstallation(const QString &path)
{
    qDebug() << "add installation in: " << path;
    Installation install;// = new Installation();

    install.hasMorrowind = findFiles(QLatin1String("Morrowind"), path);
    install.hasTribunal = findFiles(QLatin1String("Tribunal"), path);
    install.hasBloodmoon = findFiles(QLatin1String("Bloodmoon"), path);

    // Try to autodetect the Morrowind.ini location
    QDir dir(path);
    QFile file(dir.filePath("Morrowind.ini"));

    // Try the parent directory
    // In normal Morrowind installations that's where Morrowind.ini is
    if (!file.exists()) {
        dir.cdUp();
        file.setFileName(dir.filePath(QLatin1String("Morrowind.ini")));
    }

    if (file.exists())
        install.iniPath = file.fileName();

    mInstallations.insert(QDir::toNativeSeparators(path), install);

    // Add it to the openmw.cfg too
    if (!mGameSettings.getDataDirs().contains(path)) {
        mGameSettings.setMultiValue(QLatin1String("data"), path);
        mGameSettings.addDataDir(path);
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
    setPage(Page_Installation, new InstallationPage(this));
#endif
    setPage(Page_Import, new ImportPage(this));
    setPage(Page_Conclusion, new ConclusionPage(this));
    setStartId(Page_Intro);

}

void Wizard::MainWizard::importerStarted()
{
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
    QMessageBox msgBox;
    msgBox.setWindowTitle(tr("Quit Wizard"));
    msgBox.setIcon(QMessageBox::Question);
    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    msgBox.setText(tr("Are you sure you want to exit the Wizard?"));

    if (msgBox.exec() == QMessageBox::Yes) {
        QWizard::reject();
    }
}

void Wizard::MainWizard::writeSettings()
{
    // Write the encoding and language settings
    QString language(field(QLatin1String("installation.language")).toString());
    mLauncherSettings.setValue(QLatin1String("Settings/language"), language);

    if (language == QLatin1String("Polish")) {
        mGameSettings.setValue(QLatin1String("encoding"), QLatin1String("win1250"));
    } else if (language == QLatin1String("Russian")) {
        mGameSettings.setValue(QLatin1String("encoding"), QLatin1String("win1251"));
    }  else {
        mGameSettings.setValue(QLatin1String("encoding"), QLatin1String("win1252"));
    }

    // Write the installation path so that openmw can find them
    QString path(field(QLatin1String("installation.path")).toString());

    // Make sure the installation path is the last data= entry
    mGameSettings.removeDataDir(path);
    mGameSettings.addDataDir(path);

    QString userPath(toQString(mCfgMgr.getUserConfigPath()));
    QDir dir(userPath);

    if (!dir.exists()) {
        if (!dir.mkpath(userPath)) {
            QMessageBox msgBox;
            msgBox.setWindowTitle(tr("Error creating OpenMW configuration directory"));
            msgBox.setIcon(QMessageBox::Critical);
            msgBox.setStandardButtons(QMessageBox::Ok);
            msgBox.setText(tr("<html><head/><body><p><b>Could not create %1</b></p> \
                              <p>Please make sure you have the right permissions \
                              and try again.</p></body></html>").arg(userPath));
            msgBox.exec();
            return qApp->quit();
        }
    }

    // Game settings
    QFile file(userPath + QLatin1String("openmw.cfg"));

    if (!file.open(QIODevice::ReadWrite | QIODevice::Text | QIODevice::Truncate)) {
        // File cannot be opened or created
        QMessageBox msgBox;
        msgBox.setWindowTitle(tr("Error writing OpenMW configuration file"));
        msgBox.setIcon(QMessageBox::Critical);
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.setText(tr("<html><head/><body><p><b>Could not open %1 for writing</b></p> \
                          <p>Please make sure you have the right permissions \
                          and try again.</p></body></html>").arg(file.fileName()));
        msgBox.exec();
        return qApp->quit();
    }

    QTextStream stream(&file);
    stream.setCodec(QTextCodec::codecForName("UTF-8"));

    mGameSettings.writeFile(stream);
    file.close();

    // Launcher settings
    file.setFileName(userPath + QLatin1String(Config::LauncherSettings::sLauncherConfigFileName));

    if (!file.open(QIODevice::ReadWrite | QIODevice::Text | QIODevice::Truncate)) {
        // File cannot be opened or created
        QMessageBox msgBox;
        msgBox.setWindowTitle(tr("Error writing OpenMW configuration file"));
        msgBox.setIcon(QMessageBox::Critical);
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.setText(tr("<html><head/><body><p><b>Could not open %1 for writing</b></p> \
                          <p>Please make sure you have the right permissions \
                          and try again.</p></body></html>").arg(file.fileName()));
        msgBox.exec();
        return qApp->quit();
    }

    stream.setDevice(&file);
    stream.setCodec(QTextCodec::codecForName("UTF-8"));

    mLauncherSettings.writeFile(stream);
    file.close();
}

bool Wizard::MainWizard::findFiles(const QString &name, const QString &path)
{
    QDir dir(path);

    if (!dir.exists())
        return false;

    // TODO: add MIME handling to make sure the files are real
    return (dir.entryList().contains(name + QLatin1String(".esm"), Qt::CaseInsensitive)
            && dir.entryList().contains(name + QLatin1String(".bsa"), Qt::CaseInsensitive));
}

QString Wizard::MainWizard::toQString(const boost::filesystem::path& path)
{
    return QString::fromUtf8(path.string().c_str());
}

#include "mainwizard.hpp"

#include <components/process/processinvoker.hpp>

#include <QDebug>

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
#include "installationpage.hpp"
#include "importpage.hpp"
#include "conclusionpage.hpp"

using namespace Process;

Wizard::MainWizard::MainWizard(QWidget *parent) :
    mGameSettings(mCfgMgr),
    QWizard(parent)
{

#ifndef Q_OS_MAC
    setWizardStyle(QWizard::ModernStyle);
#else
    setWizardStyle(QWizard::ClassicStyle);
#endif

    setWindowTitle(tr("OpenMW Wizard"));

    // Set the property for comboboxes to the text instead of index
    setDefaultProperty("QComboBox", "currentText", "currentIndexChanged");

    setDefaultProperty("ComponentListWidget", "mCheckedItems", "checkedItemsChanged");

    setupGameSettings();
    setupInstallations();
    setupPages();
}

void Wizard::MainWizard::setupGameSettings()
{
    QString userPath(QString::fromUtf8(mCfgMgr.getUserConfigPath().string().c_str()));
    QString globalPath(QString::fromUtf8(mCfgMgr.getGlobalPath().string().c_str()));
    QString message(tr("<html><head/><body><p><b>Could not open %1 for reading</b></p> \
                    <p>Please make sure you have the right permissions \
                    and try again.</p></body></html>"));

    // Load the user config file first, separately
    // So we can write it properly, uncontaminated
    QString path(userPath + QLatin1String("openmw.cfg"));
    QFile file(path);

    qDebug() << "Loading config file:" << qPrintable(path);

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

    foreach (const QString &path, paths) {
        qDebug() << "Loading config file:" << qPrintable(path);

        QFile file(path);
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

void Wizard::MainWizard::setupInstallations()
{
    // Check if the paths actually contain a Morrowind installation
    foreach (const QString path, mGameSettings.getDataDirs()) {

        if (findFiles(QLatin1String("Morrowind"), path))
            addInstallation(path);
    }
}

void Wizard::MainWizard::runSettingsImporter()
{
    QString path(field(QLatin1String("installation.path")).toString());

    // Create the file if it doesn't already exist, else the importer will fail
    QString userPath(QString::fromUtf8(mCfgMgr.getUserConfigPath().string().c_str()));
    QFile file(userPath + QLatin1String("openmw.cfg"));

    if (!file.exists()) {
        if (!file.open(QIODevice::ReadWrite)) {
            // File cannot be created
            QMessageBox msgBox;
            msgBox.setWindowTitle(tr("Error writing OpenMW configuration file"));
            msgBox.setIcon(QMessageBox::Critical);
            msgBox.setStandardButtons(QMessageBox::Ok);
            msgBox.setText(tr("<html><head/><body><p><b>Could not open or create %1 for writing</b></p> \
                              <p>Please make sure you have the right permissions \
                              and try again.</p></body></html>").arg(file.fileName()));
            msgBox.exec();
            return qApp->quit();
        }

        file.close();
    }

    // Construct the arguments to run the importer
    QStringList arguments;

    // Import plugin selection?
    if (field(QLatin1String("installation.import-addons")).toBool() == true)
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
    arguments.append(path + QDir::separator() + QLatin1String("Morrowind.ini"));
    arguments.append(QLatin1String("--cfg"));
    arguments.append(userPath + QLatin1String("openmw.cfg"));

    if (!ProcessInvoker::startProcess(QLatin1String("mwiniimport"), arguments, false))
        return qApp->quit();;

    // Re-read the game settings
    setupGameSettings();
}

void Wizard::MainWizard::addInstallation(const QString &path)
{
    qDebug() << "add installation in: " << path;
    Installation* install = new Installation();

    install->hasMorrowind = findFiles(QLatin1String("Morrowind"), path);
    install->hasTribunal = findFiles(QLatin1String("Tribunal"), path);
    install->hasBloodmoon = findFiles(QLatin1String("Bloodmoon"), path);

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
        install->iniPath = file.fileName();

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
    setPage(Page_Installation, new InstallationPage(this));
    setPage(Page_Import, new ImportPage(this));
    setPage(Page_Conclusion, new ConclusionPage(this));
    setStartId(Page_Intro);
}

void Wizard::MainWizard::accept()
{
    writeSettings();
    QWizard::accept();
}

void Wizard::MainWizard::writeSettings()
{
    QString path(field(QLatin1String("installation.path")).toString());

    // Make sure the installation path is the last data= entry
    mGameSettings.removeDataDir(path);
    mGameSettings.addDataDir(path);

    QString userPath(QString::fromUtf8(mCfgMgr.getUserConfigPath().string().c_str()));
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

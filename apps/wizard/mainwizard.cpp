#include "mainwizard.hpp"

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

    setupInstallations();
    setupPages();
}

void Wizard::MainWizard::setupInstallations()
{
    QString userPath(QFile::decodeName(mCfgMgr.getUserPath().string().c_str()));
    QString globalPath(QFile::decodeName(mCfgMgr.getGlobalPath().string().c_str()));

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
                msgBox.setText(QObject::tr("<br><b>Could not open %0 for reading</b><br><br> \
                                           Please make sure you have the right permissions \
                                           and try again.<br>").arg(file.fileName()));
                                           msgBox.exec();
                return qApp->quit();
            }
            QTextStream stream(&file);
            stream.setCodec(QTextCodec::codecForName("UTF-8"));

            mGameSettings.readFile(stream);
        }
        file.close();
    }

    // Check if the paths actually contain data files
    foreach (const QString path, mGameSettings.getDataDirs()) {
        QDir dir(path);
        QStringList filters;
        filters << "*.esp" << "*.esm" << "*.omwgame" << "*.omwaddon";

        // Add to Wizard installations
        if (!dir.entryList(filters).isEmpty())
            addInstallation(path);
    }

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
    mGameSettings.setMultiValue(QString("data"), path);
    mGameSettings.addDataDir(path);
}

void Wizard::MainWizard::setupPages()
{
    setPage(Page_Intro, new IntroPage(this));
    setPage(Page_MethodSelection, new MethodSelectionPage(this));
    setPage(Page_LanguageSelection, new LanguageSelectionPage(this));
    setPage(Page_ExistingInstallation, new ExistingInstallationPage(this));
    setPage(Page_InstallationTarget, new InstallationTargetPage(this));
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
    QString userPath(QFile::decodeName(mCfgMgr.getUserPath().string().c_str()));
    QFile file(userPath + QLatin1String("openmw.cfg"));

    if (!file.open(QIODevice::ReadWrite | QIODevice::Text | QIODevice::Truncate)) {
        // File cannot be opened or created
        QMessageBox msgBox;
        msgBox.setWindowTitle(tr("Error writing OpenMW configuration file"));
        msgBox.setIcon(QMessageBox::Critical);
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.setText(tr("<br><b>Could not open or create %0 for writing</b><br><br> \
                          Please make sure you have the right permissions \
                          and try again.<br>").arg(file.fileName()));
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
    if (dir.exists(name + QLatin1String(".esm")) && dir.exists(name + QLatin1String(".bsa")))
    {
        return true;
    } else {
        return false;
    }

}

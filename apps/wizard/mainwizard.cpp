#include "mainwizard.hpp"

#include <QDebug>
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
    // TODO: detect existing installations
    QStringList paths;
    paths << QString("/home/pvdk/.wine/drive_c/Program Files/Bethesda Softworks/Morrowind");
    paths << QString("/home/pvdk/openmw/Data Files");
    paths << QString("/usr/games/morrowind");

    foreach (const QString &path, paths)
    {
        Installation* install = new Installation();

        install->hasMorrowind = (findFiles(QString("Morrowind"), path));
        install->hasTribunal = true;
        install->hasBloodmoon = false;

        mInstallations.insert(QDir::toNativeSeparators(path), install);
    }

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

bool Wizard::MainWizard::findFiles(const QString &name, const QString &path)
{
    QDir dir(path);

    if (!dir.exists())
        return false;

    if (!dir.cd(QString("Data Files")))
        return false;

    qDebug() << "name: " << name +  QString(".esm") << dir.absolutePath();

    // TODO: add MIME handling to make sure the files are real
    if (dir.exists(name + QString(".esm")) && dir.exists(name + QString(".bsa")))
    {
        return true;
    } else {
        return false;
    }

}

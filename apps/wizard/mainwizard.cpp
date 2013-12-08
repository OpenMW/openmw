#include "mainwizard.hpp"
#include "intropage.hpp"
#include "methodselectionpage.hpp"
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
    setupPages();
}

void Wizard::MainWizard::setupPages()
{
    setPage(Page_Intro, new IntroPage);
    setPage(Page_MethodSelection, new MethodSelectionPage);
    setPage(Page_ExistingInstallation, new ExistingInstallationPage);
    setPage(Page_InstallationTarget, new InstallationTargetPage);
    setPage(Page_ComponentSelection, new ComponentSelectionPage);
    setPage(Page_Installation, new InstallationPage);
    setPage(Page_Import, new ImportPage);
    setPage(Page_Conclusion, new ConclusionPage);
    setStartId(Page_Intro);
}

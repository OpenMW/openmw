#include "intropage.hpp"

#include "mainwizard.hpp"

Wizard::IntroPage::IntroPage(MainWizard *wizard) :
    QWizardPage(wizard),
    mWizard(wizard)
{
    setupUi(this);
    setPixmap(QWizard::WatermarkPixmap, QPixmap(QLatin1String(":/images/intropage-background.png")));
}

int Wizard::IntroPage::nextId() const
{
    return MainWizard::Page_MethodSelection;
}

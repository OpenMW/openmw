#include "componentselectionpage.hpp"

#include <QMessageBox>
#include <QPushButton>

#include "mainwizard.hpp"

Wizard::ComponentSelectionPage::ComponentSelectionPage(QWidget* parent)
    : QWizardPage(parent)
{
    mWizard = qobject_cast<MainWizard*>(parent);

    setupUi(this);

    setCommitPage(true);
    setButtonText(QWizard::CommitButton, tr("&Install"));

    registerField(QLatin1String("installation.installMorrowind"), morrowindCheckbox);
    registerField(QLatin1String("installation.installTribunal"), tribunalCheckbox);
    registerField(QLatin1String("installation.installBloodmoon"), bloodmoonCheckbox);

    connect(morrowindCheckbox, &QCheckBox::toggled, this, &ComponentSelectionPage::updateButton);
    connect(tribunalCheckbox, &QCheckBox::toggled, this, &ComponentSelectionPage::updateButton);
    connect(bloodmoonCheckbox, &QCheckBox::toggled, this, &ComponentSelectionPage::updateButton);
}

void Wizard::ComponentSelectionPage::updateButton()
{
    if (field(QLatin1String("installation.retailDisc")).toBool())
        return;

    if (!morrowindCheckbox->isChecked() && !tribunalCheckbox->isChecked() && !bloodmoonCheckbox->isChecked())
    {
        setCommitPage(false);
        setButtonText(QWizard::NextButton, tr("&Skip"));
    }
    else
    {
        setCommitPage(true);
    }
}

void Wizard::ComponentSelectionPage::initializePage()
{
    const bool retailDisc = field(QLatin1String("installation.retailDisc")).toBool();

    bool hasMorrowind = false;
    bool hasTribunal = false;
    bool hasBloodmoon = false;
    if (!retailDisc)
    {
        const QString path = field(QLatin1String("installation.path")).toString();
        const MainWizard::Installation& installation = mWizard->mInstallations[path];
        hasMorrowind = installation.hasMorrowind;
        hasTribunal = installation.hasTribunal;
        hasBloodmoon = installation.hasBloodmoon;
    }

    morrowindCheckbox->setText(hasMorrowind ? tr("Morrowind\t\t(installed)") : tr("Morrowind"));
    morrowindCheckbox->setChecked(!hasMorrowind);
    morrowindCheckbox->setEnabled(!hasMorrowind && !retailDisc);
    tribunalCheckbox->setText(hasTribunal ? tr("Tribunal\t\t(installed)") : tr("Tribunal"));
    tribunalCheckbox->setChecked(!hasTribunal);
    tribunalCheckbox->setEnabled(!hasTribunal);
    bloodmoonCheckbox->setText(hasBloodmoon ? tr("Bloodmoon\t\t(installed)") : tr("Bloodmoon"));
    bloodmoonCheckbox->setChecked(!hasBloodmoon);
    bloodmoonCheckbox->setEnabled(!hasBloodmoon);
}

bool Wizard::ComponentSelectionPage::validatePage()
{
    if (field(QLatin1String("installation.retailDisc")).toBool())
        return true;

    const QString path = field(QLatin1String("installation.path")).toString();
    MainWizard::Installation& installation = mWizard->mInstallations[path];

    bool installingTribunal = field(QLatin1String("installation.installTribunal")).toBool();
    bool installingBloodmoon = field(QLatin1String("installation.installBloodmoon")).toBool();

    if (installingTribunal && !installingBloodmoon && installation.hasBloodmoon)
    {
        QMessageBox msgBox;
        msgBox.setWindowTitle(tr("About to install Tribunal after Bloodmoon"));
        msgBox.setIcon(QMessageBox::Information);
        msgBox.setStandardButtons(QMessageBox::Cancel);
        msgBox.setText(
            tr("<html><head/><body><p><b>You are about to install Tribunal</b></p>"
               "<p>Bloodmoon is already installed on your computer.</p>"
               "<p>However, it is recommended that you install Tribunal before Bloodmoon.</p>"
               "<p>Would you like to re-install Bloodmoon?</p></body></html>"));

        QAbstractButton* reinstallButton = msgBox.addButton(tr("Re-install &Bloodmoon"), QMessageBox::ActionRole);
        msgBox.exec();

        if (msgBox.clickedButton() == reinstallButton)
        {
            installation.hasBloodmoon = false;
            bloodmoonCheckbox->setText(tr("Bloodmoon"));
            bloodmoonCheckbox->setChecked(true);
            bloodmoonCheckbox->setEnabled(true);
        }
    }

    return true;
}

int Wizard::ComponentSelectionPage::nextId() const
{
#ifdef OPENMW_USE_UNSHIELD
    if (isCommitPage())
        return MainWizard::Page_Installation;
#endif
    return MainWizard::Page_Import;
}

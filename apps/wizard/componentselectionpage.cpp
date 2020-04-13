#include "componentselectionpage.hpp"

#include <QDebug>
#include <QPushButton>
#include <QMessageBox>

#include "mainwizard.hpp"

Wizard::ComponentSelectionPage::ComponentSelectionPage(QWidget *parent) :
    QWizardPage(parent)
{
    mWizard = qobject_cast<MainWizard*>(parent);

    setupUi(this);

    setCommitPage(true);
    setButtonText(QWizard::CommitButton, tr("&Install"));

    registerField(QLatin1String("installation.components"), componentsList);

    connect(componentsList, SIGNAL(itemChanged(QListWidgetItem *)),
            this, SLOT(updateButton(QListWidgetItem *)));

}

void Wizard::ComponentSelectionPage::updateButton(QListWidgetItem*)
{
    if (field(QLatin1String("installation.retailDisc")).toBool() == true)
        return; // Morrowind is always checked here

    bool unchecked = true;

    for (int i =0; i < componentsList->count(); ++i) {
        QListWidgetItem *item = componentsList->item(i);

        if (!item)
            continue;

        if (item->checkState() == Qt::Checked) {
            unchecked = false;
        }
    }

    if (unchecked) {
        setCommitPage(false);
        setButtonText(QWizard::NextButton, tr("&Skip"));
    } else {
        setCommitPage(true);
    }
}

void Wizard::ComponentSelectionPage::initializePage()
{
    componentsList->clear();

    QString path(field(QLatin1String("installation.path")).toString());

    QListWidgetItem *morrowindItem = new QListWidgetItem(QLatin1String("Morrowind"));
    QListWidgetItem *tribunalItem = new QListWidgetItem(QLatin1String("Tribunal"));
    QListWidgetItem *bloodmoonItem = new QListWidgetItem(QLatin1String("Bloodmoon"));

    if (field(QLatin1String("installation.retailDisc")).toBool() == true)
    {
        morrowindItem->setFlags((morrowindItem->flags() & ~Qt::ItemIsEnabled) | Qt::ItemIsUserCheckable);
        morrowindItem->setData(Qt::CheckStateRole, Qt::Checked);
        componentsList->addItem(morrowindItem);

        tribunalItem->setFlags(tribunalItem->flags() | Qt::ItemIsUserCheckable);
        tribunalItem->setData(Qt::CheckStateRole, Qt::Checked);
        componentsList->addItem(tribunalItem);

        bloodmoonItem->setFlags(bloodmoonItem->flags() | Qt::ItemIsUserCheckable);
        bloodmoonItem->setData(Qt::CheckStateRole, Qt::Checked);
        componentsList->addItem(bloodmoonItem);
    } else {

        if (mWizard->mInstallations[path].hasMorrowind) {
            morrowindItem->setText(tr("Morrowind\t\t(installed)"));
            morrowindItem->setFlags((morrowindItem->flags() & ~Qt::ItemIsEnabled) | Qt::ItemIsUserCheckable);
            morrowindItem->setData(Qt::CheckStateRole, Qt::Unchecked);
        } else {
            morrowindItem->setText(tr("Morrowind"));
            morrowindItem->setData(Qt::CheckStateRole, Qt::Checked);
        }

        componentsList->addItem(morrowindItem);

        if (mWizard->mInstallations[path].hasTribunal) {
            tribunalItem->setText(tr("Tribunal\t\t(installed)"));
            tribunalItem->setFlags((tribunalItem->flags() & ~Qt::ItemIsEnabled) | Qt::ItemIsUserCheckable);
            tribunalItem->setData(Qt::CheckStateRole, Qt::Unchecked);
        } else {
            tribunalItem->setText(tr("Tribunal"));
            tribunalItem->setData(Qt::CheckStateRole, Qt::Checked);
        }

        componentsList->addItem(tribunalItem);

        if (mWizard->mInstallations[path].hasBloodmoon) {
            bloodmoonItem->setText(tr("Bloodmoon\t\t(installed)"));
            bloodmoonItem->setFlags((bloodmoonItem->flags() & ~Qt::ItemIsEnabled) | Qt::ItemIsUserCheckable);
            bloodmoonItem->setData(Qt::CheckStateRole, Qt::Unchecked);
        } else {
            bloodmoonItem->setText(tr("Bloodmoon"));
            bloodmoonItem->setData(Qt::CheckStateRole, Qt::Checked);
        }

        componentsList->addItem(bloodmoonItem);
    }
}

bool Wizard::ComponentSelectionPage::validatePage()
{
    QStringList components(field(QLatin1String("installation.components")).toStringList());
    QString path(field(QLatin1String("installation.path")).toString());

//    qDebug() << components << path << mWizard->mInstallations[path];

    if (field(QLatin1String("installation.retailDisc")).toBool() == false) {
        if (components.contains(QLatin1String("Tribunal")) && !components.contains(QLatin1String("Bloodmoon")))
        {
            if (mWizard->mInstallations[path].hasBloodmoon)
            {
                QMessageBox msgBox;
                msgBox.setWindowTitle(tr("About to install Tribunal after Bloodmoon"));
                msgBox.setIcon(QMessageBox::Information);
                msgBox.setStandardButtons(QMessageBox::Cancel);
                msgBox.setText(tr("<html><head/><body><p><b>You are about to install Tribunal</b></p> \
                                  <p>Bloodmoon is already installed on your computer.</p> \
                                  <p>However, it is recommended that you install Tribunal before Bloodmoon.</p> \
                                  <p>Would you like to re-install Bloodmoon?</p></body></html>"));

                QAbstractButton *reinstallButton = msgBox.addButton(tr("Re-install &Bloodmoon"), QMessageBox::ActionRole);
                msgBox.exec();


                if (msgBox.clickedButton() == reinstallButton) {
                    // Force reinstallation
                    mWizard->mInstallations[path].hasBloodmoon = false;
                    QList<QListWidgetItem*> items = componentsList->findItems(QLatin1String("Bloodmoon"), Qt::MatchStartsWith);

                    for (QListWidgetItem *item : items)
                    {
                        item->setText(QLatin1String("Bloodmoon"));
                        item->setCheckState(Qt::Checked);
                    }

                    return true;
                }
            }
        }
    }

    return true;
}

int Wizard::ComponentSelectionPage::nextId() const
{
#ifdef OPENMW_USE_UNSHIELD
    if (isCommitPage()) {
        return MainWizard::Page_Installation;
    } else {
        return MainWizard::Page_Import;
    }
#else
    return MainWizard::Page_Import;
#endif
}

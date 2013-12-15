#include "componentselectionpage.hpp"

#include <QDebug>
#include <QAbstractButton>

#include "mainwizard.hpp"

Wizard::ComponentSelectionPage::ComponentSelectionPage(MainWizard *wizard) :
    QWizardPage(wizard),
    mWizard(wizard)
{
    setupUi(this);

    setCommitPage(true);
    setButtonText(QWizard::CommitButton, tr("&Install"));

    registerField("installation.components", componentsList);

    connect(componentsList, SIGNAL(itemChanged(QListWidgetItem *)),
            this, SLOT(updateButton(QListWidgetItem *)));

}

void Wizard::ComponentSelectionPage::debugMe(QString &text)
{
    qDebug() << "Debug Me" << text;
}

void Wizard::ComponentSelectionPage::updateButton(QListWidgetItem *item)
{
    if (field("installation.new").toBool() == true)
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

    QString path = field("installation.path").toString();

    QListWidgetItem *morrowindItem = new QListWidgetItem(QLatin1String("Morrowind"));
    QListWidgetItem *tribunalItem = new QListWidgetItem(QLatin1String("Tribunal"));
    QListWidgetItem *bloodmoonItem = new QListWidgetItem(QLatin1String("Bloodmoon"));

    if (field("installation.new").toBool() == true)
    {
        morrowindItem->setFlags(morrowindItem->flags() & !Qt::ItemIsEnabled & Qt::ItemIsUserCheckable);
        morrowindItem->setData(Qt::CheckStateRole, Qt::Checked);
        componentsList->addItem(morrowindItem);

        tribunalItem->setFlags(tribunalItem->flags() | Qt::ItemIsUserCheckable);
        tribunalItem->setData(Qt::CheckStateRole, Qt::Checked);
        componentsList->addItem(tribunalItem);

        bloodmoonItem->setFlags(bloodmoonItem->flags() | Qt::ItemIsUserCheckable);
        bloodmoonItem->setData(Qt::CheckStateRole, Qt::Checked);
        componentsList->addItem(bloodmoonItem);
    } else {

        if (mWizard->mInstallations[path]->hasMorrowind == true) {
            morrowindItem->setText(tr("Morrowind\t\t(installed)"));
            morrowindItem->setFlags(morrowindItem->flags() & !Qt::ItemIsEnabled & Qt::ItemIsUserCheckable);
            morrowindItem->setData(Qt::CheckStateRole, Qt::Unchecked);
        } else {
            morrowindItem->setText(tr("Morrowind"));
            morrowindItem->setData(Qt::CheckStateRole, Qt::Checked);
        }

        componentsList->addItem(morrowindItem);

        if (mWizard->mInstallations[path]->hasTribunal == true) {
            tribunalItem->setText(tr("Tribunal\t\t(installed)"));
            tribunalItem->setFlags(tribunalItem->flags() & !Qt::ItemIsEnabled & Qt::ItemIsUserCheckable);
            tribunalItem->setData(Qt::CheckStateRole, Qt::Unchecked);
        } else {
            tribunalItem->setText(tr("Tribunal"));
            tribunalItem->setData(Qt::CheckStateRole, Qt::Checked);
        }

        componentsList->addItem(tribunalItem);

        if (mWizard->mInstallations[path]->hasBloodmoon == true) {
            bloodmoonItem->setText(tr("Bloodmoon\t\t(installed)"));
            bloodmoonItem->setFlags(bloodmoonItem->flags() & !Qt::ItemIsEnabled & Qt::ItemIsUserCheckable);
            bloodmoonItem->setData(Qt::CheckStateRole, Qt::Unchecked);
        } else {
            bloodmoonItem->setText(tr("Bloodmoon"));
            bloodmoonItem->setData(Qt::CheckStateRole, Qt::Checked);
        }

        componentsList->addItem(bloodmoonItem);
    }
}

int Wizard::ComponentSelectionPage::nextId() const
{
    if (isCommitPage())
        return MainWizard::Page_Installation;

    return MainWizard::Page_Import;
}

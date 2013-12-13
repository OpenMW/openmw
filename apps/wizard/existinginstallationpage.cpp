#include "existinginstallationpage.hpp"

#include <QDebug>
#include <QFileDialog>

#include "mainwizard.hpp"

Wizard::ExistingInstallationPage::ExistingInstallationPage(MainWizard *wizard) :
    QWizardPage(wizard),
    mWizard(wizard)
{
    setupUi(this);

    connect(detectedList, SIGNAL(currentTextChanged(QString)),
            this, SLOT(textChanged(QString)));

    connect(detectedList,SIGNAL(itemSelectionChanged()),
            this, SIGNAL(completeChanged()));
}

void Wizard::ExistingInstallationPage::on_browseButton_clicked()
{
    QString selectedFile = QFileDialog::getOpenFileName(
                this,
                tr("Select master file"),
                QDir::currentPath(),
                QString(tr("Morrowind master file (*.esm)")),
                NULL,
                QFileDialog::DontResolveSymlinks);

    QFileInfo info(selectedFile);
    if (!info.exists())
        return;

    QDir dir(info.absolutePath());
    if (!dir.cdUp())
        return; // Cannot move out of the Data Files directory

    QString path = QDir::toNativeSeparators(dir.absolutePath());
    QList<QListWidgetItem*> items = detectedList->findItems(path, Qt::MatchExactly);

    if (items.isEmpty())
    {
        // Path is not yet in the list, add it
        QListWidgetItem *item = new QListWidgetItem(path);
        detectedList->addItem(item);
        detectedList->setCurrentItem(item); // Select it too
    } else {
        detectedList->setCurrentItem(items.first());
    }

}

void Wizard::ExistingInstallationPage::textChanged(const QString &text)
{
    // Set the installation path manually, as registerField doesn't work
    if (!text.isEmpty())
        wizard()->setField("installation.path", text);
}

void Wizard::ExistingInstallationPage::initializePage()
{

    QStringList paths = mWizard->mInstallations.keys();

    if (paths.isEmpty())
        return;

    detectedList->clear();

    foreach (const QString &path, paths) {
        QListWidgetItem *item = new QListWidgetItem(path);
        detectedList->addItem(item);
    }

}

bool Wizard::ExistingInstallationPage::isComplete() const
{
    if (detectedList->selectionModel()->hasSelection()) {
        return true;
    } else {
        return false;
    }
}

int Wizard::ExistingInstallationPage::nextId() const
{
    QString path = field("installation.path").toString();

    if (path.isEmpty())
        return MainWizard::Page_ComponentSelection;

    if (!mWizard->mInstallations.contains(path))
        return MainWizard::Page_ComponentSelection;

    if (mWizard->mInstallations[path]->hasMorrowind == true &&
            mWizard->mInstallations[path]->hasTribunal == true &&
            mWizard->mInstallations[path]->hasBloodmoon == true)
    {
        return MainWizard::Page_Import;
    } else {
        return MainWizard::Page_ComponentSelection;
    }
}

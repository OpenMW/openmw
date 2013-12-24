#include "existinginstallationpage.hpp"

#include <QDebug>
#include <QMessageBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QFile>

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

    QString path(QDir::toNativeSeparators(info.absolutePath()));
    QList<QListWidgetItem*> items = detectedList->findItems(path, Qt::MatchExactly);

    if (items.isEmpty())
    {
        // Path is not yet in the list, add it
        mWizard->addInstallation(path);

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
        mWizard->setField("installation.path", text);
}

void Wizard::ExistingInstallationPage::initializePage()
{

    QStringList paths(mWizard->mInstallations.keys());

    if (paths.isEmpty())
        return;

    detectedList->clear();

    foreach (const QString &path, paths) {
        QListWidgetItem *item = new QListWidgetItem(path);
        detectedList->addItem(item);
    }

}

bool Wizard::ExistingInstallationPage::validatePage()
{
    // See if Morrowind.ini is detected, if not, ask the user
    // It can be missing entirely
    // Or failed to be detected due to the target being a symlink

    QString path(field("installation.path").toString());
    QFile file(mWizard->mInstallations[path]->iniPath);

    if (!file.exists()) {
        QMessageBox msgBox;
        msgBox.setWindowTitle(tr("Error detecting Morrowind configuration"));
        msgBox.setIcon(QMessageBox::Warning);
        msgBox.setStandardButtons(QMessageBox::Cancel);
        msgBox.setText(QObject::tr("<br><b>Could not find Morrowind.ini</b><br><br> \
                                   The Wizard needs to update settings in this file.<br><br> \
                                   Press \"Browse...\" to specify the location manually.<br>"));

        QAbstractButton *browseButton =
                msgBox.addButton(QObject::tr("B&rowse..."), QMessageBox::ActionRole);

        msgBox.exec();

        QString iniFile;
        if (msgBox.clickedButton() == browseButton) {
            iniFile = QFileDialog::getOpenFileName(
                        NULL,
                        QObject::tr("Select configuration file"),
                        QDir::currentPath(),
                        QString(tr("Morrowind configuration file (*.ini)")));
        }

        if (iniFile.isEmpty()) {
            return false; // Cancel was clicked;
        }

        // A proper Morrowind.ini was selected, set it
        QFileInfo info(iniFile);
        mWizard->mInstallations[path]->iniPath = info.absoluteFilePath();
    }

    return true;
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
    QString path(field("installation.path").toString());

    if (path.isEmpty())
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

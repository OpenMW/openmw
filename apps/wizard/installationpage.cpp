#include "installationpage.hpp"

#include <QDebug>
#include <QTextCodec>

#include "mainwizard.hpp"
#include "inisettings.hpp"
#include "unshield/unshieldworker.hpp"

Wizard::InstallationPage::InstallationPage(MainWizard *wizard) :
    QWizardPage(wizard),
    mWizard(wizard)
{
    setupUi(this);

    mFinished = false;
}

void Wizard::InstallationPage::initializePage()
{
    QString path(field("installation.path").toString());
    QStringList components(field("installation.components").toStringList());

    logTextEdit->append(QString("Installing to %1").arg(path));
    logTextEdit->append(QString("Installing %1.").arg(components.join(", ")));

    installProgressBar->setMinimum(0);

    // Set the progressbar maximum to a multiple of 100
    // That way installing all three components would yield 300%
    // When one component is done the bar will be filled by 33%

    if (field("installation.new").toBool() == true)
    {
        installProgressBar->setMaximum((components.count() * 100));
    }
    else
    {
        if (components.contains(QLatin1String("Tribunal"))
                && mWizard->mInstallations[path]->hasTribunal == false)
            installProgressBar->setMaximum(100);

        if (components.contains(QLatin1String("Bloodmoon"))
                && mWizard->mInstallations[path]->hasBloodmoon == false)
            installProgressBar->setMaximum(installProgressBar->maximum() + 100);
    }

    installProgressBar->setValue(100);

    startInstallation();
}

void Wizard::InstallationPage::startInstallation()
{
    QStringList components(field("installation.components").toStringList());
    QString path(field("installation.path").toString());

    QThread* thread = new QThread();
    UnshieldWorker* unshield = new UnshieldWorker();

    unshield->moveToThread(thread);

    connect(thread, SIGNAL(started()),
            unshield, SLOT(extract()));

    connect(unshield, SIGNAL(finished()),
            thread, SLOT(quit()));

    connect(unshield, SIGNAL(finished()),
            unshield, SLOT(deleteLater()));

    connect(unshield, SIGNAL(finished()),
            thread, SLOT(deleteLater()));

    connect(unshield, SIGNAL(finished()),
            this, SLOT(installationFinished()));

    connect(unshield, SIGNAL(textChanged(QString)),
            installProgressLabel, SLOT(setText(QString)));

    connect(unshield, SIGNAL(textChanged(QString)),
            logTextEdit, SLOT(append(QString)));

    connect(unshield, SIGNAL(progressChanged(int)),
            installProgressBar, SLOT(setValue(int)));

    if (field("installation.new").toBool() == true)
    {
        // Always install Morrowind
        unshield->setInstallMorrowind(true);

        if (components.contains(QLatin1String("Tribunal")))
            unshield->setInstallTribunal(true);

        if (components.contains(QLatin1String("Bloodmoon")))
            unshield->setInstallBloodmoon(true);
    } else {
        // Morrowind should already be installed
        unshield->setInstallMorrowind(false);

        if (components.contains(QLatin1String("Tribunal"))
                && mWizard->mInstallations[path]->hasTribunal == false)
            unshield->setInstallTribunal(true);

        if (components.contains(QLatin1String("Bloodmoon"))
                && mWizard->mInstallations[path]->hasBloodmoon == false)
            unshield->setInstallBloodmoon(true);

        // Set the location of the Morrowind.ini to update
        unshield->setIniPath(mWizard->mInstallations[path]->iniPath);
    }

    // Set the installation target path
    unshield->setPath(path);

    // Set the right codec to use for Morrowind.ini
    QString language(field("installation.language").toString());

    if (language == QLatin1String("Polish")) {
        unshield->setIniCodec(QTextCodec::codecForName("windows-1250"));
    }
    else if (language == QLatin1String("Russian")) {
        unshield->setIniCodec(QTextCodec::codecForName("windows-1251"));
    }
    else {
        unshield->setIniCodec(QTextCodec::codecForName("windows-1252"));
    }

    thread->start();

}

void Wizard::InstallationPage::installationFinished()
{
    qDebug() << "finished!";
    mFinished = true;
    emit completeChanged();
}

bool Wizard::InstallationPage::isComplete() const
{
    return mFinished;
}

int Wizard::InstallationPage::nextId() const
{
    return MainWizard::Page_Import;
}

#include "installationpage.hpp"

#include <QComboBox>
#include <QFileDialog>
#include <QMessageBox>
#include <QThread>

#include <components/debug/debugging.hpp>

#include "mainwizard.hpp"

Wizard::InstallationPage::InstallationPage(QWidget* parent, Config::GameSettings& gameSettings)
    : QWizardPage(parent)
    , mGameSettings(gameSettings)
{
    mWizard = qobject_cast<MainWizard*>(parent);

    setupUi(this);

    mFinished = false;

    mThread = std::make_unique<QThread>();
    mUnshield = std::make_unique<UnshieldWorker>(mGameSettings.value("morrowind-bsa-filesize").value.toLongLong());
    mUnshield->moveToThread(mThread.get());

    connect(mThread.get(), &QThread::started, mUnshield.get(), &UnshieldWorker::extract);

    connect(mUnshield.get(), &UnshieldWorker::finished, mThread.get(), &QThread::quit);

    connect(mUnshield.get(), &UnshieldWorker::finished, this, &InstallationPage::installationFinished,
        Qt::QueuedConnection);

    connect(mUnshield.get(), &UnshieldWorker::error, this, &InstallationPage::installationError, Qt::QueuedConnection);

    connect(
        mUnshield.get(), &UnshieldWorker::textChanged, installProgressLabel, &QLabel::setText, Qt::QueuedConnection);

    connect(mUnshield.get(), &UnshieldWorker::textChanged, logTextEdit, &QPlainTextEdit::appendPlainText,
        Qt::QueuedConnection);

    connect(
        mUnshield.get(), &UnshieldWorker::textChanged, this,
        [](const QString& text) { Log(Debug::Info) << qUtf8Printable(text); }, Qt::QueuedConnection);

    connect(mUnshield.get(), &UnshieldWorker::progressChanged, installProgressBar, &QProgressBar::setValue,
        Qt::QueuedConnection);

    connect(mUnshield.get(), &UnshieldWorker::requestFileDialog, this, &InstallationPage::showFileDialog,
        Qt::QueuedConnection);

    connect(mUnshield.get(), &UnshieldWorker::requestOldVersionDialog, this, &InstallationPage::showOldVersionDialog,
        Qt::QueuedConnection);
}

Wizard::InstallationPage::~InstallationPage()
{
    if (mThread->isRunning())
    {
        mUnshield->stopWorker();
        mThread->quit();
        mThread->wait();
    }
}

void Wizard::InstallationPage::initializePage()
{
    QString path(field(QLatin1String("installation.path")).toString());
    QStringList components(field(QLatin1String("installation.components")).toStringList());

    logTextEdit->appendPlainText(QString("Installing to %1").arg(path));
    logTextEdit->appendPlainText(QString("Installing %1.").arg(components.join(", ")));

    installProgressBar->setMinimum(0);

    // Set the progressbar maximum to a multiple of 100
    // That way installing all three components would yield 300%
    // When one component is done the bar will be filled by 33%

    if (field(QLatin1String("installation.retailDisc")).toBool() == true)
    {
        installProgressBar->setMaximum((components.count() * 100));
    }
    else
    {
        if (components.contains(QLatin1String("Tribunal")) && !mWizard->mInstallations[path].hasTribunal)
            installProgressBar->setMaximum(100);

        if (components.contains(QLatin1String("Bloodmoon")) && !mWizard->mInstallations[path].hasBloodmoon)
            installProgressBar->setMaximum(installProgressBar->maximum() + 100);
    }

    startInstallation();
}

void Wizard::InstallationPage::startInstallation()
{
    QStringList components(field(QLatin1String("installation.components")).toStringList());
    QString path(field(QLatin1String("installation.path")).toString());

    if (field(QLatin1String("installation.retailDisc")).toBool() == true)
    {
        // Always install Morrowind
        mUnshield->setInstallComponent(Wizard::Component_Morrowind, true);

        if (components.contains(QLatin1String("Tribunal")))
            mUnshield->setInstallComponent(Wizard::Component_Tribunal, true);

        if (components.contains(QLatin1String("Bloodmoon")))
            mUnshield->setInstallComponent(Wizard::Component_Bloodmoon, true);
    }
    else
    {
        // Morrowind should already be installed
        mUnshield->setInstallComponent(Wizard::Component_Morrowind, false);

        if (components.contains(QLatin1String("Tribunal")) && !mWizard->mInstallations[path].hasTribunal)
            mUnshield->setInstallComponent(Wizard::Component_Tribunal, true);

        if (components.contains(QLatin1String("Bloodmoon")) && !mWizard->mInstallations[path].hasBloodmoon)
            mUnshield->setInstallComponent(Wizard::Component_Bloodmoon, true);

        // Set the location of the Morrowind.ini to update
        mUnshield->setIniPath(mWizard->mInstallations[path].iniPath);
        mUnshield->setupSettings();
    }

    // Set the installation target path
    mUnshield->setPath(path);

    // Set the right codec to use for Morrowind.ini
    QString language(field(QLatin1String("installation.language")).toString());

    if (language == QLatin1String("Polish"))
    {
        mUnshield->setIniEncoding(ToUTF8::FromType::WINDOWS_1250);
    }
    else if (language == QLatin1String("Russian"))
    {
        mUnshield->setIniEncoding(ToUTF8::FromType::WINDOWS_1251);
    }
    else
    {
        mUnshield->setIniEncoding(ToUTF8::FromType::WINDOWS_1252);
    }

    mThread->start();
}

void Wizard::InstallationPage::showFileDialog(Wizard::Component component)
{
    std::string_view name;
    switch (component)
    {

        case Wizard::Component_Morrowind:
            name = "Morrowind";
            break;
        case Wizard::Component_Tribunal:
            name = "Tribunal";
            break;
        case Wizard::Component_Bloodmoon:
            name = "Bloodmoon";
            break;
    }
    logTextEdit->appendHtml(tr("<p>Attempting to install component %1.</p>").arg(QLatin1String(name)));
    Log(Debug::Info) << "Attempting to install component " << name << ".";

    QMessageBox msgBox;
    msgBox.setWindowTitle(tr("%1 Installation").arg(QLatin1String(name)));
    msgBox.setIcon(QMessageBox::Information);
    msgBox.setText(
        QObject::tr("Select a valid %1 installation media.<br><b>Hint</b>: make sure that it contains at least one "
                    "<b>.cab</b> file.")
            .arg(QLatin1String(name)));
    msgBox.exec();

    QString path = QFileDialog::getExistingDirectory(
        this, tr("Select %1 installation media").arg(QLatin1String(name)), QDir::rootPath());

    if (path.isEmpty())
    {
        logTextEdit->appendHtml(
            tr("<p><br/><span style=\"color:red;\">"
               "<b>Error: The installation was aborted by the user</b></span></p>"));

        Log(Debug::Error) << "Error: The installation was aborted by the user";
        mWizard->mError = true;

        emit completeChanged();
        return;
    }

    mUnshield->setDiskPath(path);
}

void Wizard::InstallationPage::showOldVersionDialog()
{
    logTextEdit->appendHtml(tr("<p>Detected old version of component Morrowind.</p>"));
    Log(Debug::Info) << "Detected old version of component Morrowind.";

    QMessageBox msgBox;
    msgBox.setWindowTitle(tr("Morrowind Installation"));
    msgBox.setIcon(QMessageBox::Information);
    msgBox.setText(QObject::tr(
        "There may be a more recent version of Morrowind available.<br><br>Do you wish to continue anyway?"));
    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    msgBox.setDefaultButton(QMessageBox::No);

    int ret = msgBox.exec();
    if (ret == QMessageBox::No)
    {
        logTextEdit->appendHtml(
            tr("<p><br/><span style=\"color:red;\">"
               "<b>Error: The installation was aborted by the user</b></span></p>"));

        Log(Debug::Error) << "Error: The installation was aborted by the user";
        mWizard->mError = true;

        emit completeChanged();
        return;
    }

    mUnshield->wakeAll();
}

void Wizard::InstallationPage::installationFinished()
{
    QMessageBox msgBox;
    msgBox.setWindowTitle(tr("Installation finished"));
    msgBox.setIcon(QMessageBox::Information);
    msgBox.setStandardButtons(QMessageBox::Ok);
    msgBox.setText(tr("Installation completed successfully!"));

    msgBox.exec();

    mFinished = true;
    emit completeChanged();
}

void Wizard::InstallationPage::installationError(const QString& text, const QString& details)
{
    installProgressLabel->setText(tr("Installation failed!"));

    logTextEdit->appendHtml(tr("<p><br/><span style=\"color:red;\"><b>Error: %1</b></p>").arg(text));
    logTextEdit->appendHtml(tr("<p><span style=\"color:red;\"><b>%1</b></p>").arg(details));

    Log(Debug::Error) << "Error: " << qUtf8Printable(text);
    Log(Debug::Error) << qUtf8Printable(details);

    mWizard->mError = true;
    QMessageBox msgBox;
    msgBox.setWindowTitle(tr("An error occurred"));
    msgBox.setIcon(QMessageBox::Critical);
    msgBox.setStandardButtons(QMessageBox::Ok);
    msgBox.setText(
        tr("<html><head/><body><p><b>The Wizard has encountered an error</b></p>"
           "<p>The error reported was:</p><p>%1</p>"
           "<p>Press &quot;Show Details...&quot; for more information.</p></body></html>")
            .arg(text));

    msgBox.setDetailedText(details);
    msgBox.exec();

    emit completeChanged();
}

bool Wizard::InstallationPage::isComplete() const
{
    if (!mWizard->mError)
    {
        return mFinished;
    }
    else
    {
        return true;
    }
}

int Wizard::InstallationPage::nextId() const
{
    if (field(QLatin1String("installation.retailDisc")).toBool() == true)
    {
        return MainWizard::Page_Conclusion;
    }
    else
    {
        if (!mWizard->mError)
        {
            return MainWizard::Page_Import;
        }
        else
        {
            return MainWizard::Page_Conclusion;
        }
    }
}

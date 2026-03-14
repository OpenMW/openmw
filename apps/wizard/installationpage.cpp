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
    const bool morrowind = field(QLatin1String("installation.installMorrowind")).toBool();
    const bool tribunal = field(QLatin1String("installation.installTribunal")).toBool();
    const bool bloodmoon = field(QLatin1String("installation.installBloodmoon")).toBool();

    const QString path = field(QLatin1String("installation.path")).toString();
    const MainWizard::Installation& installation = mWizard->mInstallations[path];

    QStringList installing;
    if (morrowind)
        installing << QLatin1String("Morrowind");
    if (tribunal)
        installing << QLatin1String("Tribunal");
    if (bloodmoon)
        installing << QLatin1String("Bloodmoon");

    logTextEdit->appendPlainText(QString("Installing to %1").arg(path));
    logTextEdit->appendPlainText(QString("Installing %1.").arg(installing.join(", ")));

    // Set the progressbar maximum to a multiple of 100
    // That way installing all three components would yield 300%
    // When one component is done the bar will be filled by 33%

    int steps = 0;
    if (!field(QLatin1String("installation.retailDisc")).toBool())
    {
        steps += tribunal && !installation.hasTribunal;
        steps += bloodmoon && !installation.hasBloodmoon;
    }
    else
    {
        steps = installing.count();
    }

    installProgressBar->setMinimum(0);
    installProgressBar->setMaximum(steps * 100);

    startInstallation();
}

void Wizard::InstallationPage::startInstallation()
{
    const QString path = field(QLatin1String("installation.path")).toString();

    bool hasMorrowind = false;
    bool hasTribunal = false;
    bool hasBloodmoon = false;
    if (!field(QLatin1String("installation.retailDisc")).toBool())
    {
        const MainWizard::Installation& installation = mWizard->mInstallations[path];

        // Morrowind should already be installed
        hasMorrowind = true;
        hasTribunal = installation.hasTribunal;
        hasBloodmoon = installation.hasBloodmoon;

        // Set the location of the Morrowind.ini to update
        mUnshield->setIniPath(installation.iniPath);
        mUnshield->setupSettings();
    }

    if (!hasMorrowind)
        mUnshield->setInstallComponent(Wizard::Component_Morrowind, true);

    if (!hasTribunal && field(QLatin1String("installation.installTribunal")).toBool())
        mUnshield->setInstallComponent(Wizard::Component_Tribunal, true);

    if (!hasBloodmoon && field(QLatin1String("installation.installBloodmoon")).toBool())
        mUnshield->setInstallComponent(Wizard::Component_Bloodmoon, true);

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

    QMessageBox msgBox(this);
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

    QMessageBox msgBox(this);
    msgBox.setWindowTitle(tr("Morrowind Installation"));
    msgBox.setIcon(QMessageBox::Information);
    msgBox.setText(
        QObject::tr("<br><b>There may be a more recent version of Morrowind available.</b><br><br>"
                    "Do you wish to continue anyway?<br>"));
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
    QMessageBox msgBox(this);
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
    QMessageBox msgBox(this);
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
    return mWizard->mError || mFinished;
}

int Wizard::InstallationPage::nextId() const
{
    if (!field(QLatin1String("installation.retailDisc")).toBool() && !mWizard->mError)
        return MainWizard::Page_Import;

    return MainWizard::Page_Conclusion;
}

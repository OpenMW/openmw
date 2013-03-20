#include "maindialog.hpp"

#include <QFontDatabase>
#include <QInputDialog>
#include <QFileDialog>
#include <QCloseEvent>
#include <QTextCodec>
#include <QProcess>
#include <QFile>
#include <QDir>

#include <QDebug>

#include "utils/checkablemessagebox.hpp"

#include "playpage.hpp"
#include "graphicspage.hpp"
#include "datafilespage.hpp"

MainDialog::MainDialog()
    : mGameSettings(mCfgMgr)
{
    // Install the stylesheet font
    QFile file;
    QFontDatabase fontDatabase;

    const QStringList fonts = fontDatabase.families();

    // Check if the font is installed
    if (!fonts.contains("EB Garamond")) {

        QString font = QString::fromStdString(mCfgMgr.getGlobalDataPath().string()) + QString("resources/mygui/EBGaramond-Regular.ttf");
        file.setFileName(font);

        if (!file.exists()) {
            font = QString::fromStdString(mCfgMgr.getLocalPath().string()) + QString("resources/mygui/EBGaramond-Regular.ttf");
        }

        fontDatabase.addApplicationFont(font);
    }

    setupUi(this);

    iconWidget->setViewMode(QListView::IconMode);
    iconWidget->setWrapping(false);
    iconWidget->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff); // Just to be sure
    iconWidget->setIconSize(QSize(48, 48));
    iconWidget->setMovement(QListView::Static);

    iconWidget->setSpacing(4);
    iconWidget->setCurrentRow(0);
    iconWidget->setFlow(QListView::LeftToRight);

    QPushButton *playButton = new QPushButton(tr("Play"));
    buttonBox->addButton(playButton, QDialogButtonBox::AcceptRole);

    connect(buttonBox, SIGNAL(rejected()), this, SLOT(close()));
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(play()));

    // Remove what's this? button
    setWindowFlags(this->windowFlags() & ~Qt::WindowContextHelpButtonHint);

    createIcons();
}

void MainDialog::createIcons()
{
    if (!QIcon::hasThemeIcon("document-new"))
        QIcon::setThemeName("tango");

    // We create a fallback icon because the default fallback doesn't work
    QIcon graphicsIcon = QIcon(":/icons/tango/video-display.png");

    QListWidgetItem *playButton = new QListWidgetItem(iconWidget);
    playButton->setIcon(QIcon(":/images/openmw.png"));
    playButton->setText(tr("Play"));
    playButton->setTextAlignment(Qt::AlignCenter);
    playButton->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);

    QListWidgetItem *graphicsButton = new QListWidgetItem(iconWidget);
    graphicsButton->setIcon(QIcon::fromTheme("video-display", graphicsIcon));
    graphicsButton->setText(tr("Graphics"));
    graphicsButton->setTextAlignment(Qt::AlignHCenter | Qt::AlignBottom | Qt::AlignAbsolute);
    graphicsButton->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);

    QListWidgetItem *dataFilesButton = new QListWidgetItem(iconWidget);
    dataFilesButton->setIcon(QIcon(":/images/openmw-plugin.png"));
    dataFilesButton->setText(tr("Data Files"));
    dataFilesButton->setTextAlignment(Qt::AlignHCenter | Qt::AlignBottom);
    dataFilesButton->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);

    connect(iconWidget,
            SIGNAL(currentItemChanged(QListWidgetItem*,QListWidgetItem*)),
            this, SLOT(changePage(QListWidgetItem*,QListWidgetItem*)));

}

void MainDialog::createPages()
{
    mPlayPage = new PlayPage(this);
    mGraphicsPage = new GraphicsPage(mCfgMgr, mGraphicsSettings, this);
    mDataFilesPage = new DataFilesPage(mCfgMgr, mGameSettings, mLauncherSettings, this);

    // Set the combobox of the play page to imitate the combobox on the datafilespage
    mPlayPage->setProfilesComboBoxModel(mDataFilesPage->profilesComboBoxModel());
    mPlayPage->setProfilesComboBoxIndex(mDataFilesPage->profilesComboBoxIndex());

    // Add the pages to the stacked widget
    pagesWidget->addWidget(mPlayPage);
    pagesWidget->addWidget(mGraphicsPage);
    pagesWidget->addWidget(mDataFilesPage);

    // Select the first page
    iconWidget->setCurrentItem(iconWidget->item(0), QItemSelectionModel::Select);

    connect(mPlayPage, SIGNAL(playButtonClicked()), this, SLOT(play()));

    connect(mPlayPage, SIGNAL(profileChanged(int)), mDataFilesPage, SLOT(setProfilesComboBoxIndex(int)));
    connect(mDataFilesPage, SIGNAL(profileChanged(int)), mPlayPage, SLOT(setProfilesComboBoxIndex(int)));

}

bool MainDialog::showFirstRunDialog()
{
    QStringList iniPaths;

    foreach (const QString &path, mGameSettings.getDataDirs()) {
        QDir dir(path);
        dir.setPath(dir.canonicalPath()); // Resolve symlinks

        if (!dir.cdUp())
            continue; // Cannot move from Data Files

        if (dir.exists(QString("Morrowind.ini")))
            iniPaths.append(dir.absoluteFilePath(QString("Morrowind.ini")));
    }

    // Ask the user where the Morrowind.ini is
    if (iniPaths.empty()) {
        QMessageBox msgBox;
        msgBox.setWindowTitle(tr("Error detecting Morrowind configuration"));
        msgBox.setIcon(QMessageBox::Warning);
        msgBox.setStandardButtons(QMessageBox::Cancel);
        msgBox.setText(QObject::tr("<br><b>Could not find Morrowind.ini</b><br><br> \
                                   OpenMW needs to import settings from this file.<br><br> \
                                   Press \"Browse...\" to specify the location manually.<br>"));

        QAbstractButton *dirSelectButton =
                msgBox.addButton(QObject::tr("B&rowse..."), QMessageBox::ActionRole);

        msgBox.exec();

        QString iniFile;
        if (msgBox.clickedButton() == dirSelectButton) {
            iniFile = QFileDialog::getOpenFileName(
                        NULL,
                        QObject::tr("Select configuration file"),
                        QDir::currentPath(),
                        QString(tr("Morrowind configuration file (*.ini)")));
        }

        if (iniFile.isEmpty())
            return false; // Cancel was clicked;

        QFileInfo info(iniFile);
        iniPaths.clear();
        iniPaths.append(info.absoluteFilePath());
    }

    CheckableMessageBox msgBox(this);
    msgBox.setWindowTitle(tr("Morrowind installation detected"));

    QIcon icon = QApplication::style()->standardIcon(QStyle::SP_MessageBoxQuestion);
    int size = QApplication::style()->pixelMetric(QStyle::PM_MessageBoxIconSize);
    msgBox.setIconPixmap(icon.pixmap(size, size));

    QAbstractButton *importerButton =
            msgBox.addButton(tr("Import"), QDialogButtonBox::AcceptRole); // ActionRole doesn't work?!
    QAbstractButton *skipButton =
            msgBox.addButton(tr("Skip"), QDialogButtonBox::RejectRole);

    Q_UNUSED(skipButton); // Surpress compiler unused warning

    msgBox.setStandardButtons(QDialogButtonBox::NoButton);
    msgBox.setText(tr("<br><b>An existing Morrowind configuration was detected</b><br> \
                      <br>Would you like to import settings from Morrowind.ini?<br> \
                      <br><b>Warning: In most cases OpenMW needs these settings to run properly</b><br>"));
    msgBox.setCheckBoxText(tr("Include selected masters and plugins (creates a new profile)"));
    msgBox.exec();


    if (msgBox.clickedButton() == importerButton) {

        if (iniPaths.count() > 1) {
            // Multiple Morrowind.ini files found
            bool ok;
            QString path = QInputDialog::getItem(this, tr("Multiple configurations found"),
                                                     tr("<br><b>There are multiple Morrowind.ini files found.</b><br><br> \
                                                        Please select the one you wish to import from:"), iniPaths, 0, false, &ok);
            if (ok && !path.isEmpty()) {
                iniPaths.clear();
                iniPaths.append(path);
            } else {
                // Cancel was clicked
                return false;
            }
        }

        // Create the file if it doesn't already exist, else the importer will fail
        QString path = QString::fromStdString(mCfgMgr.getUserPath().string()) + QString("openmw.cfg");
        QFile file(path);

        if (!file.exists()) {
            if (!file.open(QIODevice::ReadWrite)) {
                // File cannot be created
                QMessageBox msgBox;
                msgBox.setWindowTitle(tr("Error writing OpenMW configuration file"));
                msgBox.setIcon(QMessageBox::Critical);
                msgBox.setStandardButtons(QMessageBox::Ok);
                msgBox.setText(tr("<br><b>Could not open or create %0 for writing</b><br><br> \
                                  Please make sure you have the right permissions \
                                  and try again.<br>").arg(file.fileName()));
                msgBox.exec();
                return false;
            }

            file.close();
        }

        // Construct the arguments to run the importer
        QStringList arguments;

        if (msgBox.isChecked())
            arguments.append(QString("--game-files"));

        arguments.append(QString("--encoding"));
        arguments.append(mGameSettings.value(QString("encoding"), QString("win1252")));
        arguments.append(QString("--ini"));
        arguments.append(iniPaths.first());
        arguments.append(QString("--cfg"));
        arguments.append(path);

        if (!startProgram(QString("mwiniimport"), arguments, false))
            return false;

        // Re-read the game settings
        if (!setupGameSettings())
            return false;

        // Add a new profile
        if (msgBox.isChecked()) {
            mLauncherSettings.setValue(QString("Profiles/currentprofile"), QString("Imported"));

            mLauncherSettings.remove(QString("Profiles/Imported/master"));
            mLauncherSettings.remove(QString("Profiles/Imported/plugin"));

            QStringList masters = mGameSettings.values(QString("master"));
            QStringList plugins = mGameSettings.values(QString("plugin"));

            foreach (const QString &master, masters) {
                mLauncherSettings.setMultiValue(QString("Profiles/Imported/master"), master);
            }

            foreach (const QString &plugin, plugins) {
                mLauncherSettings.setMultiValue(QString("Profiles/Imported/plugin"), plugin);
            }
        }

    }

    return true;
}

bool MainDialog::setup()
{
    if (!setupLauncherSettings())
        return false;

    if (!setupGameSettings())
        return false;

    if (!setupGraphicsSettings())
        return false;

    // Check if we need to show the importer
    if (mLauncherSettings.value(QString("General/firstrun"), QString("true")) == QLatin1String("true"))
    {
        if (!showFirstRunDialog())
            return false;
    }

    // Now create the pages as they need the settings
    createPages();

    // Call this so we can exit on Ogre errors before mainwindow is shown
    if (!mGraphicsPage->setupOgre())
        return false;

    loadSettings();
    return true;
}

void MainDialog::changePage(QListWidgetItem *current, QListWidgetItem *previous)
{
    if (!current)
        current = previous;

    pagesWidget->setCurrentIndex(iconWidget->row(current));
}

bool MainDialog::setupLauncherSettings()
{
    QString userPath = QString::fromStdString(mCfgMgr.getUserPath().string());

    QStringList paths;
    paths.append(QString("launcher.cfg"));
    paths.append(userPath + QString("launcher.cfg"));

    foreach (const QString &path, paths) {
        qDebug() << "Loading config file:" << qPrintable(path);
        QFile file(path);
        if (file.exists()) {
            if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                QMessageBox msgBox;
                msgBox.setWindowTitle(tr("Error opening OpenMW configuration file"));
                msgBox.setIcon(QMessageBox::Critical);
                msgBox.setStandardButtons(QMessageBox::Ok);
                msgBox.setText(QObject::tr("<br><b>Could not open %0 for reading</b><br><br> \
                                  Please make sure you have the right permissions \
                                  and try again.<br>").arg(file.fileName()));
                msgBox.exec();
                return false;
            }
            QTextStream stream(&file);
            stream.setCodec(QTextCodec::codecForName("UTF-8"));

            mLauncherSettings.readFile(stream);
        }
        file.close();
    }

    return true;
}

bool MainDialog::setupGameSettings()
{
    QString userPath = QString::fromStdString(mCfgMgr.getUserPath().string());
    QString globalPath = QString::fromStdString(mCfgMgr.getGlobalPath().string());

    QStringList paths;
    paths.append(userPath + QString("openmw.cfg"));
    paths.append(QString("openmw.cfg"));
    paths.append(globalPath + QString("openmw.cfg"));

    foreach (const QString &path, paths) {
        qDebug() << "Loading config file:" << qPrintable(path);

        QFile file(path);
        if (file.exists()) {
            if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                QMessageBox msgBox;
                msgBox.setWindowTitle(tr("Error opening OpenMW configuration file"));
                msgBox.setIcon(QMessageBox::Critical);
                msgBox.setStandardButtons(QMessageBox::Ok);
                msgBox.setText(QObject::tr("<br><b>Could not open %0 for reading</b><br><br> \
                                           Please make sure you have the right permissions \
                                           and try again.<br>").arg(file.fileName()));
                                           msgBox.exec();
                return false;
            }
            QTextStream stream(&file);
            stream.setCodec(QTextCodec::codecForName("UTF-8"));

            mGameSettings.readFile(stream);
        }
        file.close();
    }

    QStringList dataDirs;

    // Check if the paths actually contain data files
    foreach (const QString path, mGameSettings.getDataDirs()) {
        QDir dir(path);
        QStringList filters;
        filters << "*.esp" << "*.esm";

        if (!dir.entryList(filters).isEmpty())
            dataDirs.append(path);
    }

    if (dataDirs.isEmpty())
    {
        QMessageBox msgBox;
        msgBox.setWindowTitle(tr("Error detecting Morrowind installation"));
        msgBox.setIcon(QMessageBox::Warning);
        msgBox.setStandardButtons(QMessageBox::Cancel);
        msgBox.setText(QObject::tr("<br><b>Could not find the Data Files location</b><br><br> \
                                   The directory containing the data files was not found.<br><br> \
                                   Press \"Browse...\" to specify the location manually.<br>"));

        QAbstractButton *dirSelectButton =
                msgBox.addButton(QObject::tr("B&rowse..."), QMessageBox::ActionRole);

        msgBox.exec();

        QString selectedFile;
        if (msgBox.clickedButton() == dirSelectButton) {
            selectedFile = QFileDialog::getOpenFileName(
                        NULL,
                        QObject::tr("Select master file"),
                        QDir::currentPath(),
                        QString(tr("Morrowind master file (*.esm)")));
        }

        if (selectedFile.isEmpty())
            return false; // Cancel was clicked;

        QFileInfo info(selectedFile);

        // Add the new dir to the settings file and to the data dir container
        mGameSettings.setMultiValue(QString("data"), info.absolutePath());
        mGameSettings.addDataDir(info.absolutePath());
    }

    return true;
}

bool MainDialog::setupGraphicsSettings()
{
    QString userPath = QString::fromStdString(mCfgMgr.getUserPath().string());
    QString globalPath = QString::fromStdString(mCfgMgr.getGlobalPath().string());

    QFile localDefault(QString("settings-default.cfg"));
    QFile globalDefault(globalPath + QString("settings-default.cfg"));

    if (!localDefault.exists() && !globalDefault.exists()) {
        QMessageBox msgBox;
        msgBox.setWindowTitle(tr("Error reading OpenMW configuration file"));
        msgBox.setIcon(QMessageBox::Critical);
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.setText(QObject::tr("<br><b>Could not find settings-default.cfg</b><br><br> \
                                   The problem may be due to an incomplete installation of OpenMW.<br> \
                                   Reinstalling OpenMW may resolve the problem."));
                                   msgBox.exec();
        return false;
    }


    QStringList paths;
    paths.append(globalPath + QString("settings-default.cfg"));
    paths.append(QString("settings-default.cfg"));
    paths.append(userPath + QString("settings.cfg"));

    foreach (const QString &path, paths) {
        qDebug() << "Loading config file:" << qPrintable(path);
        QFile file(path);
        if (file.exists()) {
            if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                QMessageBox msgBox;
                msgBox.setWindowTitle(tr("Error opening OpenMW configuration file"));
                msgBox.setIcon(QMessageBox::Critical);
                msgBox.setStandardButtons(QMessageBox::Ok);
                msgBox.setText(QObject::tr("<br><b>Could not open %0 for reading</b><br><br> \
                                           Please make sure you have the right permissions \
                                           and try again.<br>").arg(file.fileName()));
                                           msgBox.exec();
                return false;
            }
            QTextStream stream(&file);
            stream.setCodec(QTextCodec::codecForName("UTF-8"));

            mGraphicsSettings.readFile(stream);
        }
        file.close();
    }

    return true;
}

void MainDialog::loadSettings()
{
    int width = mLauncherSettings.value(QString("General/MainWindow/width")).toInt();
    int height = mLauncherSettings.value(QString("General/MainWindow/height")).toInt();

    int posX = mLauncherSettings.value(QString("General/MainWindow/posx")).toInt();
    int posY = mLauncherSettings.value(QString("General/MainWindow/posy")).toInt();

    resize(width, height);
    move(posX, posY);
}

void MainDialog::saveSettings()
{
    QString width = QString::number(this->width());
    QString height = QString::number(this->height());

    mLauncherSettings.setValue(QString("General/MainWindow/width"), width);
    mLauncherSettings.setValue(QString("General/MainWindow/height"), height);

    QString posX = QString::number(this->pos().x());
    QString posY = QString::number(this->pos().y());

    mLauncherSettings.setValue(QString("General/MainWindow/posx"), posX);
    mLauncherSettings.setValue(QString("General/MainWindow/posy"), posY);

    mLauncherSettings.setValue(QString("General/firstrun"), QString("false"));

}

bool MainDialog::writeSettings()
{
    // Now write all config files
    saveSettings();
    mGraphicsPage->saveSettings();
    mDataFilesPage->saveSettings();

    QString userPath = QString::fromStdString(mCfgMgr.getUserPath().string());
    QDir dir(userPath);

    if (!dir.exists()) {
        if (!dir.mkpath(userPath)) {
            QMessageBox msgBox;
            msgBox.setWindowTitle(tr("Error creating OpenMW configuration directory"));
            msgBox.setIcon(QMessageBox::Critical);
            msgBox.setStandardButtons(QMessageBox::Ok);
            msgBox.setText(tr("<br><b>Could not create %0</b><br><br> \
                              Please make sure you have the right permissions \
                              and try again.<br>").arg(userPath));
            msgBox.exec();
            return false;
        }
    }

    // Game settings
    QFile file(userPath + QString("openmw.cfg"));

    if (!file.open(QIODevice::ReadWrite | QIODevice::Text | QIODevice::Truncate)) {
        // File cannot be opened or created
        QMessageBox msgBox;
        msgBox.setWindowTitle(tr("Error writing OpenMW configuration file"));
        msgBox.setIcon(QMessageBox::Critical);
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.setText(tr("<br><b>Could not open or create %0 for writing</b><br><br> \
                          Please make sure you have the right permissions \
                          and try again.<br>").arg(file.fileName()));
        msgBox.exec();
        return false;
    }

    QTextStream stream(&file);
    stream.setCodec(QTextCodec::codecForName("UTF-8"));

    mGameSettings.writeFile(stream);
    file.close();

    // Graphics settings
    file.setFileName(userPath + QString("settings.cfg"));

    if (!file.open(QIODevice::ReadWrite | QIODevice::Text | QIODevice::Truncate)) {
        // File cannot be opened or created
        QMessageBox msgBox;
        msgBox.setWindowTitle(tr("Error writing OpenMW configuration file"));
        msgBox.setIcon(QMessageBox::Critical);
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.setText(tr("<br><b>Could not open or create %0 for writing</b><br><br> \
                          Please make sure you have the right permissions \
                          and try again.<br>").arg(file.fileName()));
        msgBox.exec();
        return false;
    }

    stream.setDevice(&file);
    stream.setCodec(QTextCodec::codecForName("UTF-8"));

    mGraphicsSettings.writeFile(stream);
    file.close();

    // Launcher settings
    file.setFileName(userPath + QString("launcher.cfg"));

    if (!file.open(QIODevice::ReadWrite | QIODevice::Text | QIODevice::Truncate)) {
        // File cannot be opened or created
        QMessageBox msgBox;
        msgBox.setWindowTitle(tr("Error writing Launcher configuration file"));
        msgBox.setIcon(QMessageBox::Critical);
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.setText(tr("<br><b>Could not open or create %0 for writing</b><br><br> \
                          Please make sure you have the right permissions \
                          and try again.<br>").arg(file.fileName()));
        msgBox.exec();
        return false;
    }

    stream.setDevice(&file);
    stream.setCodec(QTextCodec::codecForName("UTF-8"));

    mLauncherSettings.writeFile(stream);
    file.close();

    return true;
}

void MainDialog::closeEvent(QCloseEvent *event)
{
    writeSettings();
    event->accept();
}

void MainDialog::play()
{
    if (!writeSettings())
        qApp->quit();

    // Launch the game detached
    startProgram(QString("openmw"), true);
    qApp->quit();
}

bool MainDialog::startProgram(const QString &name, const QStringList &arguments, bool detached)
{
    QString path = name;
#ifdef Q_OS_WIN
    path.append(QString(".exe"));
#elif defined(Q_OS_MAC)
    QDir dir(QCoreApplication::applicationDirPath());
    path = dir.absoluteFilePath(name);
#else
    path.prepend(QString("./"));
#endif

    QFile file(path);

    QProcess process;
    QFileInfo info(file);

    if (!file.exists()) {
        QMessageBox msgBox;
        msgBox.setWindowTitle(tr("Error starting executable"));
        msgBox.setIcon(QMessageBox::Warning);
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.setText(tr("<br><b>Could not find %1</b><br><br> \
                        The application is not found.<br> \
                        Please make sure OpenMW is installed correctly and try again.<br>").arg(info.fileName()));
        msgBox.exec();

        return false;
    }

    if (!info.isExecutable()) {
        QMessageBox msgBox;
        msgBox.setWindowTitle(tr("Error starting executable"));
        msgBox.setIcon(QMessageBox::Warning);
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.setText(tr("<br><b>Could not start %1</b><br><br> \
                        The application is not executable.<br> \
                        Please make sure you have the right permissions and try again.<br>").arg(info.fileName()));
        msgBox.exec();

        return false;
    }

    // Start the executable
    if (detached) {
        if (!process.startDetached(path, arguments)) {
            QMessageBox msgBox;
            msgBox.setWindowTitle(tr("Error starting executable"));
            msgBox.setIcon(QMessageBox::Critical);
            msgBox.setStandardButtons(QMessageBox::Ok);
            msgBox.setText(tr("<br><b>Could not start %1</b><br><br> \
                              An error occurred while starting %1.<br><br> \
                              Press \"Show Details...\" for more information.<br>").arg(info.fileName()));
            msgBox.setDetailedText(process.errorString());
            msgBox.exec();

            return false;
        }
    } else {
        process.start(path, arguments);
        if (!process.waitForFinished()) {
            QMessageBox msgBox;
            msgBox.setWindowTitle(tr("Error starting executable"));
            msgBox.setIcon(QMessageBox::Critical);
            msgBox.setStandardButtons(QMessageBox::Ok);
            msgBox.setText(tr("<br><b>Could not start %1</b><br><br> \
                              An error occurred while starting %1.<br><br> \
                              Press \"Show Details...\" for more information.<br>").arg(info.fileName()));
            msgBox.setDetailedText(process.errorString());
            msgBox.exec();

            return false;
        }

        if (process.exitCode() != 0 || process.exitStatus() == QProcess::CrashExit) {
            QString error(process.readAllStandardError());
            error.append(tr("\nArguments:\n"));
            error.append(arguments.join(" "));

            QMessageBox msgBox;
            msgBox.setWindowTitle(tr("Error running executable"));
            msgBox.setIcon(QMessageBox::Critical);
            msgBox.setStandardButtons(QMessageBox::Ok);
            msgBox.setText(tr("<br><b>Executable %1 returned an error</b><br><br> \
                              An error occurred while running %1.<br><br> \
                              Press \"Show Details...\" for more information.<br>").arg(info.fileName()));
            msgBox.setDetailedText(error);
            msgBox.exec();

            return false;
        }
    }

    return true;

}

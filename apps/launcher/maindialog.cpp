#include <QtGui>

#include "maindialog.hpp"
#include "playpage.hpp"
#include "graphicspage.hpp"
#include "datafilespage.hpp"

MainDialog::MainDialog()
{
    QWidget *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    mIconWidget = new QListWidget(centralWidget);
    mIconWidget->setObjectName("IconWidget");
    mIconWidget->setViewMode(QListView::IconMode);
    mIconWidget->setWrapping(false);
    mIconWidget->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff); // Just to be sure
    mIconWidget->setIconSize(QSize(48, 48));
    mIconWidget->setMovement(QListView::Static);

    mIconWidget->setMinimumWidth(400);
    mIconWidget->setFixedHeight(80);
    mIconWidget->setSpacing(4);
    mIconWidget->setCurrentRow(0);
    mIconWidget->setFlow(QListView::LeftToRight);

    QGroupBox *groupBox = new QGroupBox(centralWidget);
    QVBoxLayout *groupLayout = new QVBoxLayout(groupBox);

    mPagesWidget = new QStackedWidget(groupBox);
    groupLayout->addWidget(mPagesWidget);

    QPushButton *playButton = new QPushButton(tr("Play"));

    QDialogButtonBox *buttonBox = new QDialogButtonBox(centralWidget);
    buttonBox->setStandardButtons(QDialogButtonBox::Close);
    buttonBox->addButton(playButton, QDialogButtonBox::AcceptRole);

    QVBoxLayout *dialogLayout = new QVBoxLayout(centralWidget);
    dialogLayout->addWidget(mIconWidget);
    dialogLayout->addWidget(groupBox);
    dialogLayout->addWidget(buttonBox);

    setWindowTitle(tr("OpenMW Launcher"));
    setWindowIcon(QIcon(":/images/openmw.png"));
    // Remove what's this? button
    setWindowFlags(this->windowFlags() & ~Qt::WindowContextHelpButtonHint);
    setMinimumSize(QSize(575, 575));

    // Install the stylesheet font
    QFile file;
    QFontDatabase fontDatabase;

    const QStringList fonts = fontDatabase.families();

    // Check if the font is installed
    if (!fonts.contains("EB Garamond")) {

      QString font = QString::fromStdString((mCfgMgr.getGlobalDataPath() / "resources/mygui/EBGaramond-Regular.ttf").string());
      file.setFileName(font);

      if (!file.exists()) {
      font = QString::fromStdString((mCfgMgr.getLocalPath() / "resources/mygui/EBGaramond-Regular.ttf").string());
      }

      fontDatabase.addApplicationFont(font);
    }

    // Load the stylesheet
    QString config = QString::fromStdString((mCfgMgr.getGlobalDataPath() / "resources/launcher.qss").string());
    file.setFileName(config);

    if (!file.exists()) {
        file.setFileName(QString::fromStdString((mCfgMgr.getLocalPath() / "launcher.qss").string()));
    }

    file.open(QFile::ReadOnly);
    QString styleSheet = QLatin1String(file.readAll());
    qApp->setStyleSheet(styleSheet);
    file.close();

    connect(buttonBox, SIGNAL(rejected()), this, SLOT(close()));
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(play()));

    createIcons();
    createPages();
}

void MainDialog::createIcons()
{
    if (!QIcon::hasThemeIcon("document-new")) {
        QIcon::setThemeName("tango");
    }

    // We create a fallback icon because the default fallback doesn't work
    QIcon graphicsIcon = QIcon(":/icons/tango/video-display.png");

    QListWidgetItem *playButton = new QListWidgetItem(mIconWidget);
    playButton->setIcon(QIcon(":/images/openmw.png"));
    playButton->setText(tr("Play"));
    playButton->setTextAlignment(Qt::AlignCenter);
    playButton->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);

    QListWidgetItem *graphicsButton = new QListWidgetItem(mIconWidget);
    graphicsButton->setIcon(QIcon::fromTheme("video-display", graphicsIcon));
    graphicsButton->setText(tr("Graphics"));
    graphicsButton->setTextAlignment(Qt::AlignHCenter | Qt::AlignBottom | Qt::AlignAbsolute);
    graphicsButton->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);

    QListWidgetItem *dataFilesButton = new QListWidgetItem(mIconWidget);
    dataFilesButton->setIcon(QIcon(":/images/openmw-plugin.png"));
    dataFilesButton->setText(tr("Data Files"));
    dataFilesButton->setTextAlignment(Qt::AlignHCenter | Qt::AlignBottom);
    dataFilesButton->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);

    connect(mIconWidget,
            SIGNAL(currentItemChanged(QListWidgetItem*,QListWidgetItem*)),
            this, SLOT(changePage(QListWidgetItem*,QListWidgetItem*)));

}

void MainDialog::createPages()
{
    mPlayPage = new PlayPage(this);
    mGraphicsPage = new GraphicsPage(mCfgMgr, this);
    mDataFilesPage = new DataFilesPage(mCfgMgr, this);

    // Set the combobox of the play page to imitate the combobox on the datafilespage
    mPlayPage->mProfilesComboBox->setModel(mDataFilesPage->mProfilesComboBox->model());
    mPlayPage->mProfilesComboBox->setCurrentIndex(mDataFilesPage->mProfilesComboBox->currentIndex());

    // Add the pages to the stacked widget
    mPagesWidget->addWidget(mPlayPage);
    mPagesWidget->addWidget(mGraphicsPage);
    mPagesWidget->addWidget(mDataFilesPage);

    // Select the first page
    mIconWidget->setCurrentItem(mIconWidget->item(0), QItemSelectionModel::Select);

    connect(mPlayPage->mPlayButton, SIGNAL(clicked()), this, SLOT(play()));

    connect(mPlayPage->mProfilesComboBox,
            SIGNAL(currentIndexChanged(int)),
            this, SLOT(profileChanged(int)));

    connect(mDataFilesPage->mProfilesComboBox,
            SIGNAL(currentIndexChanged(int)),
            this, SLOT(profileChanged(int)));

}


bool MainDialog::setup()
{
    // Create the settings manager and load default settings file
    const std::string localdefault = (mCfgMgr.getLocalPath() / "settings-default.cfg").string();
    const std::string globaldefault = (mCfgMgr.getGlobalPath() / "settings-default.cfg").string();

    // prefer local
    if (boost::filesystem::exists(localdefault)) {
        mSettings.loadDefault(localdefault);
    } else if (boost::filesystem::exists(globaldefault)) {
        mSettings.loadDefault(globaldefault);
    } else {
        QMessageBox msgBox;
        msgBox.setWindowTitle("Error reading OpenMW configuration file");
        msgBox.setIcon(QMessageBox::Critical);
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.setText(tr("<br><b>Could not find %0</b><br><br> \
                          The problem may be due to an incomplete installation of OpenMW.<br> \
                          Reinstalling OpenMW may resolve the problem.").arg(QString::fromStdString(globaldefault)));
        msgBox.exec();
        return false;
    }

    // load user settings if they exist, otherwise just load the default settings as user settings
    const std::string settingspath = (mCfgMgr.getUserPath() / "settings.cfg").string();

    if (boost::filesystem::exists(settingspath))
        mSettings.loadUser(settingspath);
    else if (boost::filesystem::exists(localdefault))
        mSettings.loadUser(localdefault);
    else if (boost::filesystem::exists(globaldefault))
        mSettings.loadUser(globaldefault);

    // Setup the Graphics page
    if (!mGraphicsPage->setupOgre()) {
        return false;
    }

    // Setup the Data Files page
    if (!mDataFilesPage->setupDataFiles()) {
        return false;
    }

    return true;
}

void MainDialog::profileChanged(int index)
{
    // Just to be sure, should always have a selection
    if (!mIconWidget->selectionModel()->hasSelection()) {
        return;
    }

    QString currentPage = mIconWidget->currentItem()->data(Qt::DisplayRole).toString();
    if (currentPage == QLatin1String("Play")) {
        mDataFilesPage->mProfilesComboBox->setCurrentIndex(index);
    }

    if (currentPage == QLatin1String("Data Files")) {
        mPlayPage->mProfilesComboBox->setCurrentIndex(index);
    }
}

void MainDialog::changePage(QListWidgetItem *current, QListWidgetItem *previous)
{
    if (!current)
        current = previous;

    mPagesWidget->setCurrentIndex(mIconWidget->row(current));
}

void MainDialog::closeEvent(QCloseEvent *event)
{
    // Now write all config files
    mDataFilesPage->writeConfig();
    mGraphicsPage->writeConfig();

    // Save user settings
    const std::string settingspath = (mCfgMgr.getUserPath() / "settings.cfg").string();
    mSettings.saveUser(settingspath);

    event->accept();
}

void MainDialog::play()
{
    // First do a write of all the configs, just to be sure
    mDataFilesPage->writeConfig();
    mGraphicsPage->writeConfig();

    // Save user settings
    const std::string settingspath = (mCfgMgr.getUserPath() / "settings.cfg").string();
    mSettings.saveUser(settingspath);

#ifdef Q_WS_WIN
    QString game = "./openmw.exe";
    QFile file(game);
#elif defined(Q_WS_MAC)
    QDir dir(QCoreApplication::applicationDirPath());
    QString game = dir.absoluteFilePath("openmw");
    QFile file(game);
    game = "\"" + game + "\"";
#else
    QString game = "./openmw";
    QFile file(game);
#endif

    QProcess process;
    QFileInfo info(file);

    if (!file.exists()) {
        QMessageBox msgBox;
        msgBox.setWindowTitle("Error starting OpenMW");
        msgBox.setIcon(QMessageBox::Warning);
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.setText(tr("<br><b>Could not find OpenMW</b><br><br> \
                        The OpenMW application is not found.<br> \
                        Please make sure OpenMW is installed correctly and try again.<br>"));
        msgBox.exec();

        return;
    }

    if (!info.isExecutable()) {
        QMessageBox msgBox;
        msgBox.setWindowTitle("Error starting OpenMW");
        msgBox.setIcon(QMessageBox::Warning);
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.setText(tr("<br><b>Could not start OpenMW</b><br><br> \
                        The OpenMW application is not executable.<br> \
                        Please make sure you have the right permissions and try again.<br>"));
        msgBox.exec();

        return;
    }

    // Start the game
    if (!process.startDetached(game)) {
        QMessageBox msgBox;
        msgBox.setWindowTitle("Error starting OpenMW");
        msgBox.setIcon(QMessageBox::Critical);
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.setText(tr("<br><b>Could not start OpenMW</b><br><br> \
                        An error occurred while starting OpenMW.<br><br> \
                        Press \"Show Details...\" for more information.<br>"));
        msgBox.setDetailedText(process.errorString());
        msgBox.exec();

        return;
    } else {
        qApp->quit();
    }
}


#include <QtGui>
#include <QDebug>

#include <components/files/path.hpp>

#include "maindialog.hpp"
#include "playpage.hpp"
#include "graphicspage.hpp"
#include "datafilespage.hpp"

MainDialog::MainDialog()
{
    mIconWidget = new QListWidget;
    mIconWidget->setViewMode(QListView::IconMode);
    mIconWidget->setWrapping(false);
    mIconWidget->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff); // Just to be sure
    mIconWidget->setIconSize(QSize(48, 48));
    mIconWidget->setMovement(QListView::Static);

    mIconWidget->setStyleSheet("background-image: url(:/images/openmw-header.png); \
                                background-color: white; \
                                background-repeat: no-repeat; \
                                background-attachment: scroll; \
                                background-position: right;");

    mIconWidget->setMinimumWidth(400);
    mIconWidget->setFixedHeight(80);
    mIconWidget->setSpacing(4);
    mIconWidget->setCurrentRow(0);
    mIconWidget->setFlow(QListView::LeftToRight);

    QGroupBox *groupBox = new QGroupBox(this);
    QVBoxLayout *groupLayout = new QVBoxLayout(groupBox);

    mPagesWidget = new QStackedWidget(groupBox);
    groupLayout->addWidget(mPagesWidget);

    QPushButton *playButton = new QPushButton(tr("Play"));

    QDialogButtonBox *buttonBox = new QDialogButtonBox(this);
    buttonBox->setStandardButtons(QDialogButtonBox::Close);
    buttonBox->addButton(playButton, QDialogButtonBox::AcceptRole);

    //QSpacerItem *vSpacer1 = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

    QVBoxLayout *dialogLayout = new QVBoxLayout(this);
    dialogLayout->addWidget(mIconWidget);
    //dialogLayout->addItem(vSpacer1);
    dialogLayout->addWidget(groupBox);

    dialogLayout->addWidget(buttonBox);
    //mainLayout->addStretch(1);
    //mainLayout->addSpacing(12);

    setWindowTitle(tr("OpenMW Launcher"));
    setMinimumSize(QSize(550, 450));

    connect(buttonBox, SIGNAL(rejected()), this, SLOT(close()));
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(play()));

    setupConfig();
    createIcons();
    createPages();
}

void MainDialog::createIcons()
{
    QListWidgetItem *configButton = new QListWidgetItem(mIconWidget);
    configButton->setIcon(QIcon(":/images/openmw-icon.png"));
    configButton->setText(tr("Play"));
    configButton->setTextAlignment(Qt::AlignCenter);
    configButton->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);

    QListWidgetItem *updateButton = new QListWidgetItem(mIconWidget);
    updateButton->setIcon(QIcon::fromTheme("video-display"));
    updateButton->setText(tr("Graphics"));
    updateButton->setTextAlignment(Qt::AlignHCenter | Qt::AlignBottom | Qt::AlignAbsolute);
    updateButton->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);

    QListWidgetItem *queryButton = new QListWidgetItem(mIconWidget);
    queryButton->setIcon(QIcon(":/images/openmw-plugin-icon.png"));
    queryButton->setText(tr("Data Files"));
    queryButton->setTextAlignment(Qt::AlignHCenter | Qt::AlignBottom);
    queryButton->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);

    connect(mIconWidget,
            SIGNAL(currentItemChanged(QListWidgetItem*,QListWidgetItem*)),
            this, SLOT(changePage(QListWidgetItem*,QListWidgetItem*)));

}

void MainDialog::createPages()
{
    // Various pages
    mPlayPage = new PlayPage(this);
    mGraphicsPage = new GraphicsPage(this);
    mDataFilesPage = new DataFilesPage(this);

    QString dataDir = mGameConfig->value("data").toString();
    mDataFilesPage->setupDataFiles(dataDir);

    // Add the pages to the stacked widget
    mPagesWidget->addWidget(mPlayPage);
    mPagesWidget->addWidget(mGraphicsPage);
    mPagesWidget->addWidget(mDataFilesPage);

}

void MainDialog::changePage(QListWidgetItem *current, QListWidgetItem *previous)
{
    if (!current)
        current = previous;

    mPagesWidget->setCurrentIndex(mIconWidget->row(current));

    if (previous) {
        QString previousPage = previous->data(Qt::DisplayRole).toString();
        QString currentPage = current->data(Qt::DisplayRole).toString();

        // The user switched from Data Files to Play
        if (previousPage == QString("Data Files") && currentPage == QString("Play")) {
            //mPlayPage->mProfilesModel->setStringList(mDataFilesPage->mProfilesModel->stringList());
            //mPlayPage->mProfilesComboBox->setCurrentIndex(mDataFilesPage->mProfilesComboBox->currentIndex());
        }

        // The user switched from Play to Data Files
        if (previousPage == QString("Play") && currentPage == QString("Data Files")) {
            //mDataFilesPage->mProfilesComboBox->setCurrentIndex(mPlayPage->mProfilesComboBox->currentIndex());
        }
    }

}

void MainDialog::closeEvent(QCloseEvent *event)
{
    qDebug() << "Close event";
    mDataFilesPage->writeConfig();
    mDataFilesPage->mLauncherConfig->sync();
    event->accept();

}

void MainDialog::play()
{

}

void MainDialog::setupConfig()
{
    // First we read the OpenMW config
    QString config = "openmw.cfg";
    QFile file(config);

    if (!file.exists()) {
        config = QString::fromStdString(Files::getPath(Files::Path_ConfigUser,
                                                       "openmw", "launcher.cfg"));
    }

    file.setFileName(config); // Just for displaying information
    qDebug() << "Using config file from " << file.fileName();
    file.close();

    // Open our config file
    mGameConfig = new QSettings(config, QSettings::IniFormat);


}

#include "usersettingsdialog.hpp"

#include <QApplication>
#include <QDesktopWidget>
#include <QWidget>
#include <QTabWidget>
#include <QMessageBox>
#include <QTextCodec>
#include <QFile>
#include <QPushButton>
#include <QDockWidget>

#include "blankpage.hpp"
#include "editorpage.hpp"
#include "../../model/settings/support.hpp"

#include "settingwidget.hpp"
#include <QDebug>

CSVSettings::UserSettingsDialog::UserSettingsDialog(QMainWindow *parent) :
    QMainWindow (parent), mStackedWidget (0)
{
    setWindowTitle(QString::fromUtf8 ("User Settings"));
    buildPages();
    setWidgetStates (loadSettings());
    positionWindow ();

    connect (mListWidget,
             SIGNAL (currentItemChanged(QListWidgetItem*, QListWidgetItem*)),
             this,
             SLOT (slotChangePage (QListWidgetItem*, QListWidgetItem*)));
}

CSVSettings::UserSettingsDialog::~UserSettingsDialog()
{
}

void CSVSettings::UserSettingsDialog::closeEvent (QCloseEvent *event)
{
    writeSettings();
}

void CSVSettings::UserSettingsDialog::setWidgetStates (CSMSettings::SectionMap settingsMap)
{
    //iterate the tabWidget's pages (sections)
    for (int i = 0; i < mStackedWidget->count(); i++)
    {
        //get the settings defined for the entire section
        CSMSettings::SettingMap *settings = settingsMap [mStackedWidget->widget(i)->objectName()];

        //if found, initialize the page's widgets
        if (settings)
        {
            AbstractPage *page = getAbstractPage (i);
            page->initializeWidgets(*settings);
        }
    }
}

void CSVSettings::UserSettingsDialog::buildPages()
{
    //craete central widget with it's layout and immediate children
    QWidget *centralWidget = new QWidget (this);

    mListWidget = new QListWidget (centralWidget);
    mStackedWidget = new QStackedWidget (centralWidget);

    QLayout* dialogLayout = new QHBoxLayout();

    dialogLayout->addWidget (mListWidget);
    dialogLayout->addWidget (mStackedWidget);

    centralWidget->setLayout (dialogLayout);

    setCentralWidget (centralWidget);
    setDockOptions (QMainWindow::AllowNestedDocks);
    //uncomment to test with sample editor page.
    //createSamplePage();
    createPage<BlankPage>("Page1");
    createPage<BlankPage>("Page2");
    createPage<BlankPage>("Page3");
}

void CSVSettings::UserSettingsDialog::createSamplePage()
{
    //add pages to stackedwidget and items to listwidget
    CSVSettings::AbstractPage *page
            = new CSVSettings::EditorPage(this);

    mStackedWidget->addWidget (page);

    new QListWidgetItem (page->objectName(), mListWidget);

    connect ( page, SIGNAL ( signalUpdateEditorSetting (const QString &, const QString &)),
              &(CSMSettings::UserSettings::instance()), SIGNAL ( signalUpdateEditorSetting (const QString &, const QString &)));
}

void CSVSettings::UserSettingsDialog::positionWindow ()
{
    QRect scr = QApplication::desktop()->screenGeometry();

    move(scr.center().x() - (width() / 2), scr.center().y() - (height() / 2));

}

CSMSettings::SectionMap CSVSettings::UserSettingsDialog::loadSettings ()
{
    QString userPath = QString::fromStdString(mCfgMgr.getUserPath().string());

    mPaths.append(QString("opencs.cfg"));
    mPaths.append(userPath + QString("opencs.cfg"));

    CSMSettings::SectionMap settingsMap;

    foreach (const QString &path, mPaths)
    {
        qDebug() << "Loading config file:" << qPrintable(path);
        QFile file(path);

        if (file.exists())
        {
            if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
            {
                QMessageBox msgBox;
                msgBox.setWindowTitle(tr("Error opening OpenCS configuration file"));
                msgBox.setIcon(QMessageBox::Critical);
                msgBox.setStandardButtons(QMessageBox::Ok);
                msgBox.setText(QObject::tr("<br><b>Could not open %0 for reading</b><br><br> \
                                  Please make sure you have the right permissions \
                                  and try again.<br>").arg(file.fileName()));
                msgBox.exec();
                return settingsMap;
            }

            QTextStream stream(&file);
            stream.setCodec(QTextCodec::codecForName("UTF-8"));

            CSMSettings::UserSettings::instance().getSettings(stream, settingsMap);
        }

        file.close();
    }

    return settingsMap;
}

void CSVSettings::UserSettingsDialog::writeSettings()
{
    QMap<QString, CSMSettings::SettingList *> settings;

    for (int i = 0; i < mStackedWidget->count(); ++i)
    {
        AbstractPage *page = getAbstractPage (i);
        settings [page->objectName()] = page->getSettings();
    }

    CSMSettings::UserSettings::instance().writeFile(CSMSettings::UserSettings::instance().openFile(mPaths.back()), settings);

}

CSVSettings::AbstractPage *CSVSettings::UserSettingsDialog::getAbstractPage (int index)
{
    return dynamic_cast<AbstractPage *>(mStackedWidget->widget(index));
}

void CSVSettings::UserSettingsDialog::slotChangePage(QListWidgetItem *current, QListWidgetItem *previous)
{
    if (!current)
        current = previous;

    if (!(current == previous))
        mStackedWidget->setCurrentIndex (mListWidget->row(current));
}

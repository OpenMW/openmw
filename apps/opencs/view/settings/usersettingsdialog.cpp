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
#include <QGridLayout>

#include "blankpage.hpp"
#include "samplepage.hpp"

#include "../../model/settings/support.hpp"
#include <boost/filesystem/path.hpp>
#include "settingwidget.hpp"

CSVSettings::UserSettingsDialog::UserSettingsDialog(QMainWindow *parent) :
    QMainWindow (parent), mStackedWidget (0)
{
    setWindowTitle(QString::fromUtf8 ("User Settings"));
    buildPages();
    setWidgetStates ();
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

void CSVSettings::UserSettingsDialog::setWidgetStates ()
{
    CSMSettings::UserSettings::instance().loadSettings("opencs.cfg");
    const CSMSettings::SectionMap &sectionSettings = CSMSettings::UserSettings::instance().getSettings();

    //iterate the tabWidget's pages (sections)
    for (int i = 0; i < mStackedWidget->count(); i++)
    {
        //get the settings defined for the entire section
        CSMSettings::SettingMap *settings = sectionSettings [mStackedWidget->widget(i)->objectName()];

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

    QGridLayout* dialogLayout = new QGridLayout();

    mListWidget->setMinimumWidth(0);
    mListWidget->setSizePolicy (QSizePolicy::Preferred, QSizePolicy::Expanding);

    mStackedWidget->setSizePolicy (QSizePolicy::Fixed, QSizePolicy::Fixed);

    dialogLayout->addWidget (mListWidget,0,0);
    dialogLayout->addWidget (mStackedWidget,0,1, Qt::AlignTop);

    centralWidget->setLayout (dialogLayout);

    setCentralWidget (centralWidget);
    setDockOptions (QMainWindow::AllowNestedDocks);

    //uncomment to test with sample editor page.
    // TODO:  Reimplement sample page using createPage function
    //createPage<SamplePage>("Sample");
    createPage<EditorPage>("Editor");
    createPage<BlankPage>("Page2");
    createPage<BlankPage>("Page3");
}

void CSVSettings::UserSettingsDialog::createSamplePage()
{
    //add pages to stackedwidget and items to listwidget
    CSVSettings::AbstractPage *page
            = new CSVSettings::SamplePage(this);

    mStackedWidget->addWidget (page);

    connect ( page,
              SIGNAL ( signalUpdateEditorSetting (const QString &, const QString &)),
              &(CSMSettings::UserSettings::instance()),
              SIGNAL ( signalUpdateEditorSetting (const QString &, const QString &)));

    new QListWidgetItem (page->objectName(), mListWidget);
}

void CSVSettings::UserSettingsDialog::positionWindow ()
{
    QRect scr = QApplication::desktop()->screenGeometry();

    move(scr.center().x() - (width() / 2), scr.center().y() - (height() / 2));

}

void CSVSettings::UserSettingsDialog::writeSettings()
{
    QMap<QString, CSMSettings::SettingList *> settings;

    for (int i = 0; i < mStackedWidget->count(); ++i)
    {
        AbstractPage *page = getAbstractPage (i);
        settings [page->objectName()] = page->getSettings();
    }

    CSMSettings::UserSettings::instance().writeFile(settings);

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

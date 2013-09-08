#include "usersettingsdialog.hpp"

#include <boost/filesystem/path.hpp>

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
#include <QApplication>
#include <QDesktopWidget>

#include "../../model/settings/support.hpp"

#include "datadisplayformatpage.hpp"
#include "windowpage.hpp"
#include "settingwidget.hpp"

CSVSettings::UserSettingsDialog::UserSettingsDialog(QMainWindow *parent) :
    QMainWindow (parent), mStackedWidget (0)
{
    setWindowTitle(QString::fromUtf8 ("User Settings"));
    buildPages();
    setWidgetStates ();

    connect (mListWidget,
             SIGNAL (currentItemChanged(QListWidgetItem*, QListWidgetItem*)),
             this,
             SLOT (slotChangePage (QListWidgetItem*, QListWidgetItem*)));

    QRect scr = QApplication::desktop()->screenGeometry();
    QRect rect = geometry();
    move (scr.center().x() - rect.center().x(), scr.center().y() - rect.center().y());
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

    //iterate the tabWidget's pages (sections)
    for (int i = 0; i < mStackedWidget->count(); i++)
    {
        //get the settings defined for the entire section
        //and update widget
        QString pageName = mStackedWidget->widget(i)->objectName();

        const CSMSettings::SettingMap *settings = CSMSettings::UserSettings::instance().getSettings(pageName);
        AbstractPage &page = getAbstractPage (i);
        page.initializeWidgets(*settings);
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

    createPage<WindowPage>();
    createPage<DataDisplayFormatPage>();

}

void CSVSettings::UserSettingsDialog::writeSettings()
{
    QMap<QString, CSMSettings::SettingList *> settings;

    for (int i = 0; i < mStackedWidget->count(); ++i)
    {
        AbstractPage &page = getAbstractPage (i);
        settings [page.objectName()] = page.getSettings();
    }
    CSMSettings::UserSettings::instance().writeSettings(settings);
}

CSVSettings::AbstractPage &CSVSettings::UserSettingsDialog::getAbstractPage (int index)
{
    return dynamic_cast<AbstractPage &> (*(mStackedWidget->widget (index)));
}

void CSVSettings::UserSettingsDialog::slotChangePage(QListWidgetItem *current, QListWidgetItem *previous)
{
    if (!current)
        current = previous;

    if (!(current == previous))
        mStackedWidget->setCurrentIndex (mListWidget->row(current));
}

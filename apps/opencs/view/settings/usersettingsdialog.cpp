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
#include <QDataWidgetMapper>
#include <QSortFilterProxyModel>
#include <QStandardItemModel>

#include "../../model/settings/support.hpp"

#include "../../model/settings/usersettings.hpp"
#include "settingpage.hpp"


CSVSettings::UserSettingsDialog::UserSettingsDialog(QMainWindow *parent) :
    QMainWindow (parent), mStackedWidget (0)
{
    setWindowTitle(QString::fromUtf8 ("User Settings"));
    setupStack();
    createPage ("Display Format");
    createPage ("Window Size");

    connect (mListWidget,
             SIGNAL (currentItemChanged(QListWidgetItem*, QListWidgetItem*)),
             this,
             SLOT (slotChangePage (QListWidgetItem*, QListWidgetItem*)));

    QRect scr = QApplication::desktop()->screenGeometry();
    QRect rect = geometry();
    move (scr.center().x() - rect.center().x(), scr.center().y() - rect.center().y());
}

void CSVSettings::UserSettingsDialog::setupStack()
{
    //create central widget with it's layout and immediate children
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
}

void CSVSettings::UserSettingsDialog::createPage (const QString &pageName)
{

    SettingPage *page = new SettingPage (pageName, CSMSettings::UserSettings::instance().settingModel(), false, this);

    mStackedWidget->addWidget (&dynamic_cast<QWidget &>(*(page->pageFrame())));

    new QListWidgetItem (page->objectName(), mListWidget);

    //finishing touches
    QFontMetrics fm (QApplication::font());
    int textWidth = fm.width(page->objectName());

    if ((textWidth + 50) > mListWidget->minimumWidth())
        mListWidget->setMinimumWidth(textWidth + 50);

    resize (mStackedWidget->sizeHint());
}

void CSVSettings::UserSettingsDialog::closeEvent (QCloseEvent *event)
{
    CSMSettings::UserSettings::instance().writeSettings();
}

CSVSettings::AbstractPage &CSVSettings::UserSettingsDialog::getAbstractPage (int index)
{
    //return dynamic_cast<AbstractPage &> (*(mStackedWidget->widget (index)));
}

void CSVSettings::UserSettingsDialog::slotChangePage(QListWidgetItem *current, QListWidgetItem *previous)
{
    if (!current)
        current = previous;

    if (!(current == previous))
        mStackedWidget->setCurrentIndex (mListWidget->row(current));
}

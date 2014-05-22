#include "dialog.hpp"

#include <QListWidgetItem>
#include <QApplication>
#include <QWidget>
#include <QStackedWidget>
#include <QtGui>

#include "../../model/settings/usersettings.hpp"

#include "page.hpp"

#include <QApplication>

#include <QSplitter>

#include <QTreeView>
#include <QListView>
#include <QTableView>

#include <QStandardItemModel>
#include <QStandardItem>

CSVSettings::Dialog::Dialog(QMainWindow *parent)
    : mStackedWidget (0), mDebugMode (false), SettingWindow (parent)
{
    setWindowTitle(QString::fromUtf8 ("User Settings"));

    setupDialog();

    connect (mPageListWidget,
             SIGNAL (currentItemChanged(QListWidgetItem*, QListWidgetItem*)),
             this,
             SLOT (slotChangePage (QListWidgetItem*, QListWidgetItem*)));
}

void CSVSettings::Dialog::slotChangePage
                                (QListWidgetItem *cur, QListWidgetItem *prev)
{
   mStackedWidget->changePage
                    (mPageListWidget->row (cur), mPageListWidget->row (prev));

   layout()->activate();
   setFixedSize(minimumSizeHint());
}

void CSVSettings::Dialog::setupDialog()
{
    //create central widget with it's layout and immediate children
    QWidget *centralWidget = new QGroupBox (this);

    centralWidget->setLayout (new QHBoxLayout());
    centralWidget->setSizePolicy (QSizePolicy::Expanding, QSizePolicy::Preferred);
    setCentralWidget (centralWidget);
    setDockOptions (QMainWindow::AllowNestedDocks);

    buildPageListWidget (centralWidget);
    buildStackedWidget (centralWidget);
}

void CSVSettings::Dialog::buildPages()
{
    SettingWindow::createPages ();

    QFontMetrics fm (QApplication::font());

    foreach (Page *page, SettingWindow::pages())
    {
        QString pageName = page->objectName();

        int textWidth = fm.width(pageName);

        new QListWidgetItem (pageName, mPageListWidget);
        mPageListWidget->setFixedWidth (textWidth + 50);

        mStackedWidget->addWidget (&dynamic_cast<QWidget &>(*(page)));
    }

    resize (mStackedWidget->sizeHint());
}

void CSVSettings::Dialog::buildPageListWidget (QWidget *centralWidget)
{
    mPageListWidget = new QListWidget (centralWidget);
    mPageListWidget->setMinimumWidth(50);
    mPageListWidget->setSizePolicy (QSizePolicy::Fixed, QSizePolicy::Expanding);

    mPageListWidget->setSelectionBehavior (QAbstractItemView::SelectItems);

    centralWidget->layout()->addWidget(mPageListWidget);
}

void CSVSettings::Dialog::buildStackedWidget (QWidget *centralWidget)
{
    mStackedWidget = new ResizeableStackedWidget (centralWidget);

    centralWidget->layout()->addWidget (mStackedWidget);
}

void CSVSettings::Dialog::closeEvent (QCloseEvent *event)
{
    //SettingWindow::closeEvent() must be called first to ensure
    //model is updated
    SettingWindow::closeEvent (event);

    saveSettings();
}

void CSVSettings::Dialog::show()
{
    if (pages().isEmpty())
    {
        buildPages();
        setViewValues();
    }

    QPoint screenCenter = QApplication::desktop()->screenGeometry().center();

    move (screenCenter - geometry().center());
    QWidget::show();
}

#include "dialog.hpp"

#include <algorithm>

#include <QListWidgetItem>
#include <QApplication>
#include <QWidget>
#include <QStackedWidget>
#include <QtGui>
#include <QSplitter>

#include "../../model/settings/usersettings.hpp"

#include "page.hpp"

#include <QApplication>

#include <QTreeView>
#include <QListView>
#include <QTableView>

#include <QStandardItemModel>
#include <QStandardItem>

CSVSettings::Dialog::Dialog(QMainWindow *parent)
    : mStackedWidget (0), mDebugMode (false), SettingWindow (parent)
{
    setWindowTitle(QString::fromUtf8 ("User Settings"));

    setSizePolicy (QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);

    setMinimumSize (600, 400);

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
}

void CSVSettings::Dialog::setupDialog()
{
    QSplitter *centralWidget = new QSplitter (this);
    centralWidget->setSizePolicy (QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);

    setCentralWidget (centralWidget);

    buildPageListWidget (centralWidget);
    buildStackedWidget (centralWidget);
}

void CSVSettings::Dialog::buildPages()
{
    SettingWindow::createPages ();

    QFontMetrics fm (QApplication::font());

    int maxWidth = 1;

    foreach (Page *page, SettingWindow::pages())
    {
        maxWidth = std::max (maxWidth, fm.width(page->getLabel()));

        new QListWidgetItem (page->getLabel(), mPageListWidget);

        mStackedWidget->addWidget (page);
    }

    mPageListWidget->setMaximumWidth (maxWidth + 10);

    resize (mStackedWidget->sizeHint());
}

void CSVSettings::Dialog::buildPageListWidget (QSplitter *centralWidget)
{
    mPageListWidget = new QListWidget (centralWidget);
    mPageListWidget->setMinimumWidth(50);
    mPageListWidget->setSizePolicy (QSizePolicy::Expanding, QSizePolicy::Expanding);

    mPageListWidget->setSelectionBehavior (QAbstractItemView::SelectItems);

    centralWidget->addWidget(mPageListWidget);
}

void CSVSettings::Dialog::buildStackedWidget (QSplitter *centralWidget)
{
    mStackedWidget = new ResizeableStackedWidget (centralWidget);
    mStackedWidget->setSizePolicy (QSizePolicy::Preferred, QSizePolicy::Expanding);

    centralWidget->addWidget (mStackedWidget);
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

    QWidget *currView = QApplication::activeWindow();
    if(currView)
    {
        // place at the center of the window with focus
        QSize size = currView->size();
        move(currView->geometry().x()+(size.width() - frameGeometry().width())/2,
             currView->geometry().y()+(size.height() - frameGeometry().height())/2);
    }
    else
    {
        // something's gone wrong, place at the center of the screen
        QPoint screenCenter = QApplication::desktop()->screenGeometry().center();
        move(screenCenter - QPoint(frameGeometry().width()/2,
                                   frameGeometry().height()/2));
    }
    QWidget::show();
}

#include "settingsdialog.hpp"

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

CSVSettings::SettingsDialog::SettingsDialog(QMainWindow *parent)
    : /*mStackedWidget (0),*/ mDebugMode (false), SettingWindow (parent)
{
    setWindowTitle(QString::fromUtf8 ("User Settings"));

    setupUi(this);

    //setupDialog();

    //connect (mPageListWidget,
             //SIGNAL (currentItemChanged(QListWidgetItem*, QListWidgetItem*)),
             //this,
             //SLOT (slotChangePage (QListWidgetItem*, QListWidgetItem*)));
}

void CSVSettings::SettingsDialog::slotChangePage
                                (QListWidgetItem *cur, QListWidgetItem *prev)
{
   //mStackedWidget->changePage
                    //(mPageListWidget->row (cur), mPageListWidget->row (prev));

   layout()->activate();
   setFixedSize(minimumSizeHint());
}

void CSVSettings::SettingsDialog::setupDialog()
{
    //create central widget with it's layout and immediate children
    QWidget *centralWidget = new QGroupBox (this);

    centralWidget->setLayout (new QHBoxLayout());
    centralWidget->setSizePolicy (QSizePolicy::Expanding, QSizePolicy::Preferred);
    //setCentralWidget (centralWidget);
    //setDockOptions (QMainWindow::AllowNestedDocks);

    buildPageListWidget (centralWidget);
    buildStackedWidget (centralWidget);
}

void CSVSettings::SettingsDialog::buildPages()
{
    SettingWindow::createPages ();
#if 0

    QFontMetrics fm (QApplication::font());

    foreach (Page *page, SettingWindow::pages())
    {
        QString pageName = page->objectName();

        //int textWidth = fm.width(pageName);

        //new QListWidgetItem (pageName, mPageListWidget);
        //mPageListWidget->setFixedWidth (textWidth + 50);

        //mStackedWidget->addWidget (&dynamic_cast<QWidget &>(*(page)));
    }

    //resize (mStackedWidget->sizeHint());
#endif
}

void CSVSettings::SettingsDialog::buildPageListWidget (QWidget *centralWidget)
{
    //mPageListWidget = new QListWidget (centralWidget);
    //mPageListWidget->setMinimumWidth(50);
    //mPageListWidget->setSizePolicy (QSizePolicy::Fixed, QSizePolicy::Expanding);

    //mPageListWidget->setSelectionBehavior (QAbstractItemView::SelectItems);

    //centralWidget->layout()->addWidget(mPageListWidget);
}

void CSVSettings::SettingsDialog::buildStackedWidget (QWidget *centralWidget)
{
    //mStackedWidget = new ResizeableStackedWidget (centralWidget);

    //centralWidget->layout()->addWidget (mStackedWidget);
}

void CSVSettings::SettingsDialog::closeEvent (QCloseEvent *event)
{
    //SettingWindow::closeEvent() must be called first to ensure
    //model is updated
    //SettingWindow::closeEvent (event);

    //saveSettings();
}

void CSVSettings::SettingsDialog::show()
{
    //if (pages().isEmpty())
    {
        //buildPages();
        //setViewValues();
    }
    //if(

    QPoint screenCenter = QApplication::desktop()->screenGeometry().center();

    move (screenCenter - geometry().center());
    QWidget::show();
}

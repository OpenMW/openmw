#include "dialog.hpp"

#include <QListWidgetItem>
#include <QApplication>
#include <QWidget>
#include <QStackedWidget>
#include <QtGui>

#include "../../model/settings/usersettings.hpp"

#include "page.hpp"

#include <QDebug>

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

    addDebugPage();

    resize (mStackedWidget->sizeHint());
}

void CSVSettings::Dialog::addDebugPage()
{
    QComboBox *cbox = new QComboBox();

    QStringList vals;
    vals << "1" << "2" << "3" << "4";

    QStringListModel *model = new QStringListModel (vals);
    cbox->setModel (model);

    mStackedWidget->addWidget (cbox);
    new QListWidgetItem ("test", mPageListWidget);
    /*
  QTreeView *tree = new QTreeView();

  //tree->setModel( &CSMSettings::UserSettings::instance().model() );

  mStackedWidget->addWidget(tree);
     new QListWidgetItem ("Standard Item Model", mPageListWidget);*/
}

void CSVSettings::Dialog::buildPageListWidget (QWidget *centralWidget)
{
    mPageListWidget = new QListWidget (centralWidget);
    mPageListWidget->setMinimumWidth(50);
    mPageListWidget->setSizePolicy
                            (QSizePolicy::Fixed, QSizePolicy::Expanding);

    mPageListWidget->setSelectionBehavior (QAbstractItemView::SelectItems);

    centralWidget->layout()->addWidget(mPageListWidget);
}

void CSVSettings::Dialog::buildStackedWidget (QWidget *centralWidget)
{
    mStackedWidget = new QStackedWidget (centralWidget);
    mStackedWidget->setSizePolicy (QSizePolicy::Expanding, QSizePolicy::Preferred);

    centralWidget->layout()->addWidget (mStackedWidget);
}

void CSVSettings::Dialog::closeEvent (QCloseEvent *event)
{
    //SettingWindow::closeEvent() must be called first to ensure
    //model is updated
    SettingWindow::closeEvent (event);
    CSMSettings::UserSettings::instance().saveSettings();
}

void CSVSettings::Dialog::slotChangePage(QListWidgetItem *current,
                                         QListWidgetItem *previous)
{
    if (!current)
        current = previous;

    if (!(current == previous))
        mStackedWidget->setCurrentIndex (mPageListWidget->row(current));
}

void CSVSettings::Dialog::show()
{
    if (pages().size()==0)
        buildPages();

    QRect scr = QApplication::desktop()->screenGeometry();
    QRect rect = geometry();

    move (scr.center().x()-rect.center().x(),
          scr.center().y()-rect.center().y());

    QWidget::show();
}

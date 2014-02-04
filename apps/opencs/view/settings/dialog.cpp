#include "dialog.hpp"

#include <QListWidgetItem>
#include <QApplication>
#include <QWidget>
#include <QStackedWidget>
#include <QtGui>

#include "../../model/settings/usersettings.hpp"
#include "testharnesspage.hpp"

#include "page.hpp"

#include <QDebug>

//CustomPage includes
#include <QApplication>

#include <QSplitter>

#include <QTreeView>
#include <QListView>
#include <QTableView>

#include <QStandardItemModel>

CSVSettings::Dialog::Dialog(QMainWindow *parent) :
    SettingWindow (parent), mStackedWidget (0)
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

    buildPages();
}

void CSVSettings::Dialog::buildPages()
{
    SettingWindow::createPages (CSMSettings::UserSettings::instance());

    QFontMetrics fm (QApplication::font());

    foreach (Page *page, SettingWindow::pages())
    {
        QString pageName = page->objectName();

        int textWidth = fm.width(pageName);

        new QListWidgetItem (pageName, mPageListWidget);
        mPageListWidget->setFixedWidth (textWidth + 50);

        mStackedWidget->addWidget (&dynamic_cast<QWidget &>(*(page)));
    }

    new QListWidgetItem ("Test Harness", mPageListWidget);

    mStackedWidget->addWidget (
        new TestHarnessPage (CSMSettings::UserSettings::instance(), this));

    addCustomPage();

    resize (mStackedWidget->sizeHint());
}

void CSVSettings::Dialog::addCustomPage()
{
  QTreeView *tree = new QTreeView();
  QListView *list = new QListView();
  QTableView *table = new QTableView();

  QSplitter *splitter = new QSplitter(this);
  splitter->addWidget( tree );
  splitter->addWidget( list );
  splitter->addWidget( table );

  QStandardItemModel *model = new QStandardItemModel( 5, 2 );
  for( int r=0; r<5; r++ )
    for( int c=0; c<2; c++)
    {
      QStandardItem *item = new QStandardItem( QString("Row:%0, Column:%1").arg(r).arg(c) );

      if( c == 1 )
      {
          QStringList values;
        for( int i=0; i<3; i++ )
        {
            values << QString("Item %0").arg(i);
        }
        item->appendRow (new StringListItem(values));
          //item->appendRow( new QStandardItem( QString("Item %0").arg(i) ) );
    }
      model->setItem(r, c, item);
    }

  tree->setModel( model );
  list->setModel( model );
  table->setModel( model );

  list->setSelectionModel( tree->selectionModel() );
  table->setSelectionModel( tree->selectionModel() );

  mStackedWidget->addWidget(splitter);
     new QListWidgetItem ("Standard Item Model", mPageListWidget);
  splitter->show();

  qDebug() << "values for item #2: " << model->item(2,1)->child(0,0)->data(Qt::DisplayRole).toStringList();
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
    QRect scr = QApplication::desktop()->screenGeometry();
    QRect rect = geometry();

    move (scr.center().x()-rect.center().x(),
          scr.center().y()-rect.center().y());

    QWidget::show();
}

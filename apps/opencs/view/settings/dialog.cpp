#include "dialog.hpp"

#include <boost/filesystem/path.hpp>

#include <QListWidgetItem>
#include <QApplication>
#include <QDesktopWidget>
#include <QWidget>
#include <QTabWidget>
#include <QMessageBox>
#include <QTextCodec>
#include <QFile>
#include <QStackedWidget>
#include <QGridLayout>

#include "../../model/settings/usersettings.hpp"
#include "testharnesspage.hpp"

#include "page.hpp"

#include <QDebug>

CSVSettings::Dialog::Dialog(QMainWindow *parent) :
    QMainWindow (parent), mStackedWidget (0)
{
    setWindowTitle(QString::fromUtf8 ("User Settings"));

    setupStack();

    connect (mPageListWidget,
             SIGNAL (currentItemChanged(QListWidgetItem*, QListWidgetItem*)),
             this,
             SLOT (slotChangePage (QListWidgetItem*, QListWidgetItem*)));
}

void CSVSettings::Dialog::setupStack()
{
    //create central widget with it's layout and immediate children
    QWidget *centralWidget = new QGroupBox (this);

  //  QTableView *tv = new QTableView (centralWidget);
    mPageListWidget = new QListWidget (centralWidget);
    mStackedWidget = new QStackedWidget (centralWidget);

    QHBoxLayout* dialogLayout = new QHBoxLayout();


    mPageListWidget->setMinimumWidth(50);
    mPageListWidget->setSizePolicy
                            (QSizePolicy::Fixed, QSizePolicy::Expanding);

    mPageListWidget->setSelectionBehavior (QAbstractItemView::SelectItems);

    mStackedWidget->setSizePolicy (QSizePolicy::Expanding, QSizePolicy::Fixed);


    dialogLayout->addWidget (mPageListWidget);
    dialogLayout->addWidget (mStackedWidget);

    centralWidget->setLayout (dialogLayout);

    setCentralWidget (centralWidget);
    setDockOptions (QMainWindow::AllowNestedDocks);
}

CSVSettings::Page *CSVSettings::Dialog::createPage (const QString &pageName)
{
    return new Page (pageName,
                     CSMSettings::UserSettings::instance().declarationModel(),
                     CSMSettings::UserSettings::instance().definitionModel(),
                     false, this);
}

void CSVSettings::Dialog::addPage (Page *page)
{
    mStackedWidget->addWidget (&dynamic_cast<QWidget &>(*(page)));

    new QListWidgetItem (page->objectName(), mPageListWidget);

    //finishing touches
    QFontMetrics fm (QApplication::font());
    int textWidth = fm.width(page->objectName());

    mPageListWidget->setFixedWidth(textWidth + 50);

    resize (mStackedWidget->sizeHint());
}

void CSVSettings::Dialog::closeEvent (QCloseEvent *event)
{
    CSMSettings::UserSettings::instance().saveSettings();
}

void CSVSettings::Dialog::slotChangePage(QListWidgetItem *current, QListWidgetItem *previous)
{
    if (!current)
        current = previous;

    if (!(current == previous))
        mStackedWidget->setCurrentIndex (mPageListWidget->row(current));
}

void CSVSettings::Dialog::show()
{
    addPage (createPage ("Display Format"));
    addPage (createPage ("Window Size"));
    addPage (new TestHarnessPage (
                 CSMSettings::UserSettings::instance().declarationModel(),
                 CSMSettings::UserSettings::instance().definitionModel(),
                 this));

    QRect scr = QApplication::desktop()->screenGeometry();
    QRect rect = geometry();

    move (scr.center().x()-rect.center().x(),
          scr.center().y()-rect.center().y());

    QWidget::show();
}

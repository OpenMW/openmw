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

    resize (mStackedWidget->sizeHint());
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

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
#include <QTextEdit>
#include <QTableView>

#include "../../model/settings/support.hpp"
#include "../../model/settings/binarywidgetadapter.hpp"
#include "../../model/settings/usersettings.hpp"
#include "../../model/settings/sectionfilter.hpp"

#include "settingpage.hpp"

#include <QDebug>

CSVSettings::UserSettingsDialog::UserSettingsDialog(QMainWindow *parent) :
    QMainWindow (parent), mStackedWidget (0)
{
    return;
    createPage ("Display Format");
    createPage ("Window Size");
/*
    connect (mListWidget,
             SIGNAL (currentItemChanged(QListWidgetItem*, QListWidgetItem*)),
             this,
             SLOT (slotChangePage (QListWidgetItem*, QListWidgetItem*)));
*/
    QRect scr = QApplication::desktop()->screenGeometry();
    QRect rect = geometry();
    move (scr.center().x() - rect.center().x(), scr.center().y() - rect.center().y());
}

void CSVSettings::UserSettingsDialog::setupStack()
{
    //create central widget with it's layout and immediate children
    QWidget *centralWidget = new QWidget (this);

    QTableView *tv = new QTableView (centralWidget);
    //mListView = new QListView (centralWidget);
    mStackedWidget = new QStackedWidget (centralWidget);

    QGridLayout* dialogLayout = new QGridLayout();

    tv->setMinimumWidth(50);
    tv->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
    tv->setSelectionBehavior(QAbstractItemView::SelectItems);
    tv->setModel(CSMSettings::UserSettings::instance().settingModel());
    //mListView->setMinimumWidth(50);
    //mListView->setSizePolicy (QSizePolicy::Preferred, QSizePolicy::Expanding);
    //mListView->setSelectionBehavior(QAbstractItemView::SelectItems);

    mStackedWidget->setSizePolicy (QSizePolicy::Fixed, QSizePolicy::Fixed);

    dialogLayout->addWidget(tv, 0, 0);
    //dialogLayout->addWidget (mListView,0,0);
    dialogLayout->addWidget (mStackedWidget,0,1, Qt::AlignTop);

    centralWidget->setLayout (dialogLayout);

    setCentralWidget (centralWidget);
    setDockOptions (QMainWindow::AllowNestedDocks);
}

void CSVSettings::UserSettingsDialog::createPage (const QString &pageName)
{

    SettingPage *page = new SettingPage (pageName,
            CSMSettings::UserSettings::instance().settingModel(), false, this);

    mStackedWidget->addWidget (&dynamic_cast<QWidget &>(*(page->pageFrame())));

   // new QListWidgetItem (page->objectName(), mListWidget);

    //finishing touches
    QFontMetrics fm (QApplication::font());
    int textWidth = fm.width(page->objectName());

    //mListWidget->setMinimumWidth(textWidth + 50);

    resize (mStackedWidget->sizeHint());
}

void CSVSettings::UserSettingsDialog::closeEvent (QCloseEvent *event)
{
    CSMSettings::UserSettings::instance().writeSettings();
}

void CSVSettings::UserSettingsDialog::slotChangePage(QListWidgetItem *current, QListWidgetItem *previous)
{
    if (!current)
        current = previous;

  //  if (!(current == previous))
        //mStackedWidget->setCurrentIndex (mListWidget->row(current));
}

void CSVSettings::UserSettingsDialog::show()
{
    setWindowTitle(QString::fromUtf8 ("User Settings"));
    setupStack();

    QGroupBox *myGroupBox = new QGroupBox(this);
    myGroupBox->setLayout(new QVBoxLayout());

    mStackedWidget->addWidget(myGroupBox);
    testMapperRadioButton(myGroupBox);
    testMapperCheckBox(myGroupBox);

    QWidget::show();
}

void testLineEditWidget()
{

}

void CSVSettings::UserSettingsDialog::testMapperCheckBox(QGroupBox *gb)
{
    CSMSettings::SectionFilter *filter  =
            new CSMSettings::SectionFilter ("Display Format", this);

    CSMSettings::BinaryWidgetAdapter *bin =
            new CSMSettings::BinaryWidgetAdapter("Display Format",
                                                   "Record Status Display",
                                                   this);
    bin->setObjectName("check_box_bin");
    QStringList valueList = bin->valueList();
    int j  = 0;

    QTableView *tv = new QTableView (gb);

    tv->setMinimumWidth(50);
    tv->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
    tv->setSelectionBehavior(QAbstractItemView::SelectItems);
    tv->setModel(bin->filter());

    gb->layout()->addWidget(tv);

    QTableView *tv2 = new QTableView (gb);

    tv2->setMinimumWidth(50);
    tv2->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
    tv2->setSelectionBehavior(QAbstractItemView::SelectItems);
    tv2->setModel(bin);

    gb->layout()->addWidget(tv2);
    foreach (const QString &item, valueList)
    {
        QCheckBox *widget  = new QCheckBox(item, this);
        widget->setObjectName(item);
        gb->layout()->addWidget (widget);
        QDataWidgetMapper *mapper = new QDataWidgetMapper(widget);
        mapper->setModel(bin);
        mapper->addMapping(widget, 1);
        mapper->setCurrentIndex(j);
        j++;
    }
}

void CSVSettings::UserSettingsDialog::testMapperRadioButton(QGroupBox *gb)
{
    CSMSettings::SectionFilter *filter =
            new CSMSettings::SectionFilter ("Display Format", this);

    CSMSettings::BinaryWidgetAdapter *bin =
            new CSMSettings::BinaryWidgetAdapter("Display Format",
                                                   "Record Status Display",
                                                   this);
    bin->setObjectName("radio_button_bin");
    bin->setSingleValueMode(true);
    gb->setLayout (new QVBoxLayout());
    QStringList valueList = bin->valueList();

    int j = 0;
    foreach (const QString &item, valueList)
    {
        QRadioButton *widget = new QRadioButton(item, this);
        widget->setObjectName(item);
        gb->layout()->addWidget (widget);
        QDataWidgetMapper *mapper = new QDataWidgetMapper(widget);
        mapper->setModel(bin);
        mapper->addMapping(widget, 1);
        mapper->setCurrentIndex(j);
        j++;
    }
}

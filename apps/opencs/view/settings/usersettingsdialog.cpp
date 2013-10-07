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
#include "blankpage.hpp"
#include "../../model/settings/usersettings.hpp"

#include <QDataWidgetMapper>
#include <QSortFilterProxyModel>
#include <QStandardItemModel>

CSVSettings::UserSettingsDialog::UserSettingsDialog(QMainWindow *parent) :
    QMainWindow (parent), mStackedWidget (0)
{
    setWindowTitle(QString::fromUtf8 ("User Settings"));
    buildPages();

    createSettingModelWidget ();

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

void CSVSettings::UserSettingsDialog::createSettingModelWidget()
{

}

void CSVSettings::UserSettingsDialog::closeEvent (QCloseEvent *event)
{
    CSMSettings::UserSettings::instance().writeSettings();
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
    createPage<BlankPage>();

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

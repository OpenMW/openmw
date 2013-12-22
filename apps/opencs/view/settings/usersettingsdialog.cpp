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

#include "../../model/settings/support.hpp"
#include "../../model/settings/binarywidgetfilter.hpp"
#include "../../model/settings/usersettings.hpp"

#include "settingpage.hpp"

#include <QDebug>

CSVSettings::UserSettingsDialog::UserSettingsDialog(QMainWindow *parent) :
    QMainWindow (parent), mStackedWidget (0)
{
    return;
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

    mListWidget->setMinimumWidth(50);
    mListWidget->setSizePolicy (QSizePolicy::Preferred, QSizePolicy::Expanding);
    mListWidget->setSelectionBehavior(QAbstractItemView::SelectItems);

    mStackedWidget->setSizePolicy (QSizePolicy::Fixed, QSizePolicy::Fixed);

    dialogLayout->addWidget (mListWidget,0,0);
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

    new QListWidgetItem (page->objectName(), mListWidget);

    //finishing touches
    QFontMetrics fm (QApplication::font());
    int textWidth = fm.width(page->objectName());

    mListWidget->setMinimumWidth(textWidth + 50);

    resize (mStackedWidget->sizeHint());
}

void CSVSettings::UserSettingsDialog::closeEvent (QCloseEvent *event)
{
    CSMSettings::UserSettings::instance().writeSettings();
}

void CSVSettings::UserSettingsDialog::slotChangePage(QListWidgetItem *current, QListWidgetItem *previous)
{
    qDebug() << "page change!";
    if (!current)
        current = previous;

    if (!(current == previous))
        mStackedWidget->setCurrentIndex (mListWidget->row(current));
}

void CSVSettings::UserSettingsDialog::show()
{
    setWindowTitle(QString::fromUtf8 ("User Settings"));
    setupStack();

    testMapperCheckBox();

    QWidget::show();
}

void CSVSettings::UserSettingsDialog::testMapperCheckBox()
{

    CSMSettings::Setting *setting =
            CSMSettings::UserSettings::instance().settingModel()->getSetting
                            ("Display Format", "Referenceable ID Type Display");

    QGroupBox *widgetBox = new QGroupBox(this);
    QTextEdit *textEdit = new QTextEdit(this);
    widgetBox->setLayout (new QVBoxLayout());

    foreach (const QString &item, setting->valueList())
    {
        UsdWidget *button = new UsdWidget (this);
        button->setWidget(new QCheckBox(item, this));

        widgetBox->layout()->addWidget (button->widget());

        CSMSettings::BinaryWidgetFilter *binFilter =
                new CSMSettings::BinaryWidgetFilter(item, button->widget());

        binFilter->setSourceModel(&setting->valueModel());

        int mapperIndex = -1;

        for (int i=0; i < binFilter->rowCount(); ++i)
        {
            QModelIndex idx = binFilter->index(i, 0, QModelIndex());
            if (binFilter->data(idx, Qt::DisplayRole).toBool())
            {
                mapperIndex = i;
                break;
            }
        }

        qDebug() << "index for item: " << item << " = " << mapperIndex;

        QDataWidgetMapper *mapper = new QDataWidgetMapper(button->widget());
        button->setMapper(mapper);
        mapper->setObjectName(item + "_mapper");

        mapper->setModel(binFilter);
        mapper->addMapping(button->widget(), 0);
        mapper->toFirst();

        for (int i = 0; i < mapperIndex; i++)
            mapper->toNext();

        mapper->setSubmitPolicy(QDataWidgetMapper::ManualSubmit);

        button->index = mapperIndex;

        connect (button, SIGNAL (toggled(bool)), this, SLOT (slotReadout(bool)));
    }

    widgetBox->layout()->addWidget(textEdit);
    mStackedWidget->addWidget(widgetBox);

}

void CSVSettings::UserSettingsDialog::slotReadout (bool toggled)
{
    QStringList vals = CSMSettings::UserSettings::instance().settingModel()->
            getSetting("Display Format", "Referenceable ID Type Display")->values();

    UsdWidget *widg = static_cast<UsdWidget *>(QObject::sender());

    QDataWidgetMapper *mapper = widg->mapper();

    if (toggled)
        widg->index = vals.count();
    else
        widg->index = -1;

    mapper->setCurrentIndex(widg->index);
    mapper->submit();


    qDebug() << "slotReadOut()::widget toggled to " << toggled;
    qDebug() << "slotReadOut()::value list: " << vals;
    qDebug() << "slotReadOut()::mapper index: " << widg->index;
}

void CSVSettings::UserSettingsDialog::testMapperRadioButton()
{

    QSortFilterProxyModel *filter = 0; //testMapperModelSetup("Display Format",
                                     //                   "Referenceable ID Type Display");

    QStringList valueList = filter->data(filter->index(0,4,QModelIndex())).toStringList();

    QGroupBox *widgetBox = new QGroupBox(this);
    widgetBox->setLayout (new QVBoxLayout());

    for (int i = 0; i < valueList.count(); ++i)
    {
        QString text = valueList.at(i);

        QRadioButton * button = new QRadioButton(text);

        widgetBox->layout()->addWidget (button);

        CSMSettings::BinaryWidgetFilter *binFilter =
                new CSMSettings::BinaryWidgetFilter(text, button);

        binFilter->setSourceModel(filter);

        QDataWidgetMapper *mapper = new QDataWidgetMapper(button);
        mapper->setModel(binFilter);
        mapper->addMapping(button, 2);
        mapper->toFirst();
        mapper->setSubmitPolicy(QDataWidgetMapper::ManualSubmit);

        connect (button, SIGNAL (released()), mapper, SLOT (submit()));
    }

    mStackedWidget->addWidget(widgetBox);
}

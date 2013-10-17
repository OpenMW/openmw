#include <QSortFilterProxyModel>
#include <QHBoxLayout>

#include "settingview.hpp"
#include "settingwidget.hpp"
#include "widgetfactory.hpp"
#include "apps/opencs/model/settings/setting.hpp"
#include "support.hpp"

CSVSettings::SettingView::SettingView (const QString &viewName, WidgetType widgetType, bool isHorizontal, QWidget *parent) :
    QGroupBox(parent), mWidgetFactory (0)
 {
    setObjectName (viewName);
    setTitle (viewName);
    setupView (isHorizontal);
    makeFactory (widgetType);
 }

void CSVSettings::SettingView::buildWidgets ()
{
     //retrieve the value list for the setting
    QStringList valueList = mSettingFilter->data (mSettingIndices.at(4)).toStringList();

    //for non-list widgets with value lists, create a widget for each item in the list
    //each widget label takes one of the values from the list
     if (valueList.size() > 0)
     {
         AbstractWidget *widget = mWidgetFactory->createWidget(valueList.at(0), layout());

        // if (!(widget->isListWidget()))
         {
             for (int i = 1; i < valueList.size(); ++i)
                 mWidgetFactory->createWidget(valueList.at(i), layout());

             return;
         }
     }

     QString name = mSettingFilter->data (mSettingIndices.at(0)).toString();

     // pass-through to create widgets that are either list-widgets or have no value list
     mWidgetFactory->createWidget(name, layout());
}

void CSVSettings::SettingView::setModel (QSortFilterProxyModel *settingModel)
{
    mSettingFilter = new QSortFilterProxyModel (this);
    mSettingFilter->setFilterFixedString (objectName());
    mSettingFilter->setFilterKeyColumn (0);
    mSettingFilter->setSourceModel(settingModel);
    mSettingFilter->setDynamicSortFilter (true);

    //create setting indices
    for (int i = 0; i < CSMSettings::Setting::columnCount(); ++i)
        mSettingIndices.append(mSettingFilter->index(0, i, QModelIndex()));


    mDataAdapter = new QDataWidgetMapper (parent());
    mFilterProxy = new QSortFilterProxyModel (parent());

    mFilterProxy->setSourceModel (model);
    mFilterProxy->setFilterKeyColumn (0);
    mFilterProxy->setFilterFixedString (objectName());
    mFilterProxy->setDynamicSortFilter (true);

    qDebug() << "building widget based on section: " << objectName() << "; records: " << mFilterProxy->rowCount();

    qDebug() << "record value: " << mFilterProxy->data(mFilterProxy->index(0,2,QModelIndex()), Qt::DisplayRole).toString();
    mDataAdapter->setModel (mFilterProxy);

    buildWidgets();
}

void CSVSettings::SettingView::makeFactory (WidgetType widgetType)
{
    switch (widgetType)
    {
    case Widget_CheckBox:
        mWidgetFactory = WidgetFactory::checkBox(layout(), this);
        break;

    case Widget_ComboBox:
        mWidgetFactory = new TypedWidgetFactory<QComboBox>(layout(), this);
        break;

    case Widget_SpinBox:
        mWidgetFactory = new TypedWidgetFactory<QSpinBox>(layout(), this);
        break;

    case Widget_LineEdit:
        mWidgetFactory = new TypedWidgetFactory<QLineEdit>(layout(), this);
        break;

    case Widget_ListBox:
        mWidgetFactory = new TypedWidgetFactory<QListWidget>(layout(), this);
        break;

    case Widget_RadioButton:
        mWidgetFactory = new TypedWidgetFactory<QRadioButton>(layout(), this);
        break;

    case Widget_ToggleButton:
      //  mWidgetFactory = new TypedWidgetFactory<ToggleButton>(layout(), this);
        break;

    default:
        break;
    }
}

//view layout code


const QString CSVSettings::SettingView::INVISIBLE_BOX_STYLE =
        QString::fromUtf8("QSettingBox { border: 0px; padding 0px; margin: 0px;}");

void CSVSettings::SettingView::setupView(bool isHorizontal)
{
    setFlat (true);
    mVisibleBoxStyle = styleSheet();

    setStyleSheet (INVISIBLE_BOX_STYLE);

    if (isHorizontal)
        setLayout (new QHBoxLayout);
    else
        setLayout (new QVBoxLayout);
}

bool CSVSettings::SettingView::borderVisibile() const
{
    return (styleSheet() != INVISIBLE_BOX_STYLE);
}

void CSVSettings::SettingView::setTitle (const QString &title)
{
    if (borderVisibile() )
    {
        QGroupBox::setTitle (title);
        setMinimumWidth();
    }
}

void CSVSettings::SettingView::setBorderVisibility (bool value)
{
    if (value)
        setStyleSheet(mVisibleBoxStyle);
    else
        setStyleSheet(INVISIBLE_BOX_STYLE);
}

void CSVSettings::SettingView::setMinimumWidth()
{
    //set minimum width to accommodate title, if needed
    //1.5 multiplier to account for bold title.
    QFontMetrics fm (font());
    int minWidth = fm.width(title());
    QGroupBox::setMinimumWidth (minWidth * 1.5);
}

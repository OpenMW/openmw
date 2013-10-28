#include <QDataWidgetMapper>
#include <QHBoxLayout>
#include <QLabel>
#include <QCheckBox>
#include <QComboBox>
#include <QListWidget>
#include <QSpinBox>
#include <QRadioButton>
#include <QLineEdit>
#include <QSortFilterProxyModel>
#include <QVBoxLayout>

#ifdef Q_OS_MAC
#include <QPlastiqueStyle>
#endif

#include <QDebug>

#include "widgetfactory.hpp"
#include "../../model/settings/setting.hpp"
#include "../../model/settings/settingmodel.hpp"
#include "../../model/settings/binarywidgetfilter.hpp"

CSVSettings::SettingWidget::SettingWidget (const QString &name, QWidget *parent = 0)
: QWidget (parent)
{
    mWidget = new T (parent);
    mIsBinary = mWidget->property("isChecked").isValid();
    mHasLabel = (dynamic_cast<QAbstractButton *>(mWidget));

    setLayout (new QHBoxLayout());

    layout()->setContentsMargins (0, 0, 0, 0);

    if (!mHasLabel)
    {
        QLabel *label = new QLabel (name, parent);
        label->setBuddy (widget);
        layout()->addWidget (label);
    }

    layout()->addWidget (widget);
}

CSVSettings::WidgetFactory::WidgetFactory (QSortFilterProxyModel *model, QWidget *parent)
    : mParent (parent), mSourceModel (model)
{}

QLayout *CSVSettings::WidgetFactory::buildLayout (QSortFilterProxyModel *model)
{
    //retrieve setting
    const CSMSettings::Setting *setting = getSetting (model);

    //create one instance of the widget type
    SettingWidget *widget = createWidget (setting->name(), setting->widgetType());

    //abort if fail
    if (!widget)
        return 0;

    // if the widget is binary and there is a value list,
    // generate additional widgets for each value in the value list
    if (setting->valueList().size() > 1 && widget->isBinary())
    {
        QLayout *layout = buildLayout ( setting->isHorizontal() );

        foreach (const QString &settingValue, setting->valueList())
        {
            if (!widget)
                widget = createWidget (settingValue, setting->widgetType());
            else
                widget->setObjectName (settingValue);

            configureWidget (widget, setting);
            layout->addWidget (widget);

            buildModelConnections (model, widget);

            widget = 0;
        }
    }



    if (!setting)
    {
        QWarning() << "Unable to find setting for widget creation";
        return;
    }

    buildWidget (widget, setting);
    buildMapper (widget, filter);
    return buildLayout(widget, setting);
/*
    qDebug() << "building widget based on section: " << objectName() << "; records: " << mFilterProxy->rowCount();

    qDebug() << "record value: " << mFilterProxy->data(mFilterProxy->index(0,2,QModelIndex()), Qt::DisplayRole).toString();
  */
}

CSVSettings::SettingWidget *CSVSettings::WidgetFactory::createWidget (const QString &name, CSVSettings::WidgetType widgetType)
{
    switch (widgetType)
    {
    case Widget_CheckBox:
        return new SettingWidget<QCheckBox> (name, mParent);
        break;

    case Widget_ComboBox:
        return new SettingWidget<QComboBox> (name, mParent);
        break;

    case Widget_LineEdit:
        return new SettingWidget<QLineEdit> (name, mParent);
        break;

    case Widget_ListBox:
        return new SettingWidget<QListWidget> (name, mParent);
        break;

    case Widget_RadioButton:
        return new SettingWidget<QRadioButton> (name, mParent);
        break;

    case Widget_SpinBox:
        return new SettingWidget<QSpinBox> (name, mParent);
        break;

    case Widget_ToggleButton:
        break;

    default:
        break;
    }

    return 0;
}

const CSMSettings::Setting *getSetting (QSortFilterProxyModel *model) const
{
    int row = model->mapToSource(model->index (0, 0, QModelIndex())).row();

    return static_cast<CSMSettings::SettingModel *>(model->sourceModel())->getSetting (row);
}

QWidget *configureWidget (QWidget *widget, CSMSettings::Setting *setting)
{
    if (widget->property ("setInputMask").isValid())
        widget->setProperty ("setInputMask", setting->inputMask());

}

QSortFilterProxyModel *CSVSettings::WidgetFactory::buildModelConnections (QSortFilterProxyModel *model, SettingWidget *widget)
{

}

QSortFilterProxyModel *CSVSettings::WidgetFactory::buildModel(QWidget *widget, const QString &name)
{
    QSortFilterProxyModel *filter = 0;

    if (widget->property("isChecked").isValid())
    {
        filter =
    }
    else
        filter = new QSortFilterProxyModel (mParent);

    filter->setSourceModel (mSourceModel);
    filter->setFilterKeyColumn (0);
    filter->setFilterFixedString (name);
    filter->setDynamicSortFilter (true);

    return filter;
}

void CSVSettings::WidgetFactory::buildMapper (QWidget *widget, QSortFilterProxyModel *filter)
{
    QDataWidgetMapper mapper = new QDataWidgetMapper (mParent);
    mapper->setModel (filter);
    mapper->addMapping (widget, 2);
    mapper->toFirst();
}

QLayout *CSVSettings::WidgetFactory::buildLayout (bool isHoriztonal)
{
    QLayout *layout = 0;

    if (isHorizontal)
        layout = new QHBoxLayout();
    else
        layout = new QVBoxLayout();

    layout->setContentsMargins (0, 0, 0, 0);

    return layout;
}


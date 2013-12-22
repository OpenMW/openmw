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

#include "settingfactory.hpp"
#include "../../model/settings/setting.hpp"
#include "../../model/settings/settingmodel.hpp"
#include "../../model/settings/binarywidgetfilter.hpp"

CSVSettings::SettingFactory::Setting::Setting(QWidget *parent)
    : mHasLabel (false), mIsBinary (false), QWidget (parent)
{}

void CSVSettings::SettingFactory::Setting::configureView(const QString& name,
                                                                QWidget *widget)
{
    setObjectName(name);
    mWidget = widget;

    //create and configure layout
    setLayout (new QHBoxLayout());
    layout()->setContentsMargins (0, 0, 0, 0);

    mIsBinary = mWidget->property("isChecked").isValid();
    mHasLabel = (dynamic_cast<QAbstractButton *>(mWidget));


    //if no label, create it
    if (!mHasLabel)
    {
        QLabel *label = new QLabel (name, widget);
        label->setBuddy (mWidget);
        layout()->addWidget (label);
    }
    else
        dynamic_cast<QAbstractButton *>(mWidget)->setText(name);

    layout()->addWidget (mWidget);
}

void CSVSettings::SettingFactory::Setting::configureModel
                                                (QSortFilterProxyModel *model)
{
    QSortFilterProxyModel *filter = new QSortFilterProxyModel();
    QDataWidgetMapper *mapper = new QDataWidgetMapper (mWidget);

    filter->setSourceModel(model);
    filter->setFilterFixedString(mWidget->objectName());
    filter->setDynamicSortFilter(true);

    mapper->setModel (filter);
    mapper->addMapping (mWidget, 2);
    mapper->toFirst();
}

CSVSettings::SettingFactory::Setting
    *CSVSettings::SettingFactory::createSetting(WidgetType type,
                QSortFilterProxyModel *model, const QString &fieldName)
{
    switch (type)
    {
    case Widget_CheckBox:
        return Setting::create<QCheckBox> (fieldName, model, mParent);
        break;

    case Widget_ComboBox:
        return Setting::create<QComboBox> (fieldName, model, mParent);
        break;

    case Widget_LineEdit:
        return Setting::create<QLineEdit> (fieldName, model, mParent);
        break;

    case Widget_ListBox:
        return Setting::create<QListWidget> (fieldName, model, mParent);
        break;

    case Widget_RadioButton:
        return Setting::create<QRadioButton> (fieldName, model, mParent);
        break;

    case Widget_SpinBox:
        return Setting::create<QSpinBox> (fieldName, model, mParent);
        break;

    case Widget_ToggleButton:
        break;

    default:
        break;
    }
    return 0;
}
/*
QWidget *configureWidget (QWidget *widget, CSMSettings::Setting *setting)
{
    if (widget->property ("setInputMask").isValid())
        widget->setProperty ("setInputMask", setting->inputMask());

}*/

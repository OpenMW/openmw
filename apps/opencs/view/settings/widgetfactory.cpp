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

void CSVSettings::SettingFactory::Setting::configure(const QString &name,
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
        QLabel *label = new QLabel (name, parent);
        label->setBuddy (mWidget);
        layout()->addWidget (label);
    }

    layout()->addWidget (mWidget);
}

QWidget *CSVSettings::SettingFactory::createSetting(WidgetType type,
                QSortFilterProxyModel *model, const QString &settingFieldName)
{
    Setting* setting = createWidget (settingFieldName, type);
    QSortFilterProxyModel filter = buildFilter(setting, model);
    buildSettingMap();
    return setting;
}

QWidget *CSVSettings::SettingFactory::createWidget (const QString &name,
                                            CSVSettings::WidgetType widgetType)
{
    switch (widgetType)
    {
    case Widget_CheckBox:
        return Setting::create<QCheckBox> (name, mParent);
        break;

    case Widget_ComboBox:
        return Setting::create<QComboBox> (name, mParent);
        break;

    case Widget_LineEdit:
        return Setting::create<QLineEdit> (name, mParent);
        break;

    case Widget_ListBox:
        return Setting::create<QListWidget> (name, mParent);
        break;

    case Widget_RadioButton:
        return Setting::create<QRadioButton> (name, mParent);
        break;

    case Widget_SpinBox:
        return Setting::create<QSpinBox> (name, mParent);
        break;

    case Widget_ToggleButton:
        break;

    default:
        break;
    }

    return 0;
}

QWidget *configureWidget (QWidget *widget, CSMSettings::Setting *setting)
{
    if (widget->property ("setInputMask").isValid())
        widget->setProperty ("setInputMask", setting->inputMask());

}

QSortFilterProxyModel *CSVSettings::SettingFactory::buildModel(
        Setting *setting, QSortFilterProxyModel model)
{
    QSortFilterProxyModel *filter = new QSortFilterProxyModel(setting);

    filter->setSourceModel (model);
    filter->setFilterFixedString (setting->objectName());
    filter->setDynamicSortFilter (true);

    return filter;
}

void CSVSettings::SettingFactory::buildSettingMap (QWidget *widget,
                                                  QSortFilterProxyModel *filter)
{
    QDataWidgetMapper mapper = new QDataWidgetMapper (widget);
    mapper->setModel (filter);
    mapper->addMapping (widget, 2);
    mapper->toFirst();
}


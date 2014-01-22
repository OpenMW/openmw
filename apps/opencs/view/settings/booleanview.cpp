#include <QHBoxLayout>
#include <QVBoxLayout>

#include <QCheckBox>
#include <QRadioButton>
#include <QGroupBox>

#include <QDataWidgetMapper>

#include "booleanview.hpp"
#include "../../model/settings/booleanadapter.hpp"
#include "../../model/settings/setting.hpp"
#include "../../model/settings/definitionmodel.hpp"

#include <QDebug>

CSVSettings::BooleanView::BooleanView (QAbstractItemModel *booleanAdapter,
                                       const CSMSettings::Setting *setting,
                                       QWidget *parent)
    : View (booleanAdapter, setting, parent)
{

    build (setting->settingName);
}

void CSVSettings::BooleanView::build(const QString &settingName)
{
    createView (settingName);
    createModel (settingName);
}

void CSVSettings::BooleanView::createView(const QString &settingName)
{
    setObjectName (settingName + "_view");

    foreach (const QString &value, valueList())
    {
        QWidget *widget = 0;

        if (isMultiValue())
            widget = new QCheckBox (value, this);
        else
            widget = new QRadioButton (value, this);

        widget->setObjectName (value);

        viewFrame()->layout()->addWidget (widget);

        mWidgets.append (widget);
    }
}

void CSVSettings::BooleanView::createModel (const QString &settingName)
{
    int i = 0;

    foreach (QWidget *widget, mWidgets)
    {
        QDataWidgetMapper *mapper = new QDataWidgetMapper (widget);
        mapper->setObjectName
                    (settingName + '.' + widget->objectName() + "_mapper");
        mapper->setModel (model());
        mapper->addMapping (widget, CSMSettings::BooleanSetting_ValueState);
        mapper->setCurrentIndex(i);
        i++;
    }
}

CSVSettings::BooleanView *CSVSettings::BooleanViewFactory::createView
    (CSMSettings::DefinitionModel &model, const CSMSettings::Setting *setting)
{
    QWidget *widgParent = static_cast<QWidget *>(parent());

    CSMSettings::BooleanAdapter *adapter =
            new CSMSettings::BooleanAdapter (model, setting, widgParent);

    return new BooleanView (adapter, setting, widgParent);
}

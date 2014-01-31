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
#include "settingbox.hpp"

#include <QDebug>

CSVSettings::BooleanView::BooleanView (QAbstractItemModel *booleanAdapter,
                                       const CSMSettings::Setting *setting,
                                       QWidget *parent)
    : View (booleanAdapter, setting, parent)
{
    createView (setting);
    createModel (setting->settingName);
}

void CSVSettings::BooleanView::createView(const CSMSettings::Setting *setting)
{
    setObjectName (setting->settingName + "_view");

    foreach (const QString &value, setting->valueList)
    {
        QWidget *widget = 0;

        if (isMultiValue())
            widget = new QCheckBox (value, this);
        else
            widget = new QRadioButton (value, this);

        widget->setObjectName (value);

        viewFrame()->addWidget (widget);

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

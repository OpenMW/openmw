#include <QHBoxLayout>
#include <QVBoxLayout>

#include <QCheckBox>
#include <QRadioButton>
#include <QGroupBox>

#include <QDataWidgetMapper>
#include <QStandardItemModel>
#include <QStringListModel>

#include "booleanview.hpp"
#include "../../model/settings/booleanadapter.hpp"
#include "../../model/settings/setting.hpp"
#include "settingbox.hpp"

#include <QDebug>

CSVSettings::BooleanView::BooleanView (const CSMSettings::Setting &setting,
                                       CSMSettings::Adapter *adapter,
                                       QWidget *parent)
    : View (setting, adapter, parent)
{
    createView (setting);
    createModel (setting);
}

void CSVSettings::BooleanView::createView(const CSMSettings::Setting &setting)
{
   // setObjectName (setting->settingName + "_view");

    QStringList declaration = setting.propertyList
                                    (CSMSettings::PropertyList_DeclaredValues);

    foreach (const QString &value, declaration)
    {
        QWidget *widget = 0;

        if (setting.isMultiValue())
            widget = new QCheckBox (value, this);
        else
            widget = new QRadioButton (value, this);

        widget->setObjectName (value);

        viewFrame()->addWidget (widget);

        mWidgets.append (widget);
    }
}

void CSVSettings::BooleanView::createModel (const CSMSettings::Setting &setting)
{
    int i = 0;

    mModel = new QStringListModel (setting.declarationList());

    foreach (QWidget *widget, mWidgets)
    {
        QDataWidgetMapper *mapper = new QDataWidgetMapper (widget);
        mapper->setObjectName
                    (setting.name() + '.' + widget->objectName() + "_mapper");
        mapper->setModel (mModel);
        mapper->addMapping (widget, CSMSettings::BooleanProperty_ValueState);
        mapper->setCurrentIndex(i);
        i++;
    }
}

CSVSettings::BooleanView *CSVSettings::BooleanViewFactory::createView
    (const CSMSettings::Setting &setting)
{
    QWidget *widgParent = static_cast<QWidget *>(parent());

    CSMSettings::BooleanAdapter *adapter =
            new CSMSettings::BooleanAdapter (model, setting, widgParent);

    return new BooleanView (setting, adapter, widgParent);
}

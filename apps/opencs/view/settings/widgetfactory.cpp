#include <QDataWidgetMapper>
#include <QHBoxLayout>
#include <QLabel>
#include <QSortFilterProxyModel>
#include <QVBoxLayout>

#ifdef Q_OS_MAC
#include <QPlastiqueStyle>
#endif

#include <QDebug>

#include "widgetfactory.hpp"
#include "../../model/settings/setting.hpp"
#include "../../model/settings/settingmodel.hpp"

CSVSettings::WidgetFactory::WidgetFactory (QSortFilterProxyModel *model, QWidget *parent)
    : mParent (parent), mSourceModel (model)
{}

QLayout *CSVSettings::WidgetFactory::build (QWidget *widget, CSMSettings::Setting *setting)
{
    QSortFilterProxyModel *filter = buildModel (widget, name);
    int row = filter->mapToSource(filter->index (0, 0, QModelIndex())).row();
    CSMSettings::Setting *setting = static_cast<CSMSettings::SettingModel *>(filter->sourceModel())->getSetting (row);

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

void buildWidget (QWidget *widget, CSMSettings::Setting *setting)
{
    if (widget->property ("setInputMask").isValid())
        widget->setProperty ("setInputMask", setting->inputMask());

    widget->

}

QSortFilterProxyModel *CSVSettings::WidgetFactory::buildModel(QWidget *widget, const QString &name)
{
    QSortFilterProxyModel *filter = 0;

    if (widget->property("isChecked").isValid())
    {
        //create custom filter here for binary widgets
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

QLayout *CSVSettings::WidgetFactory::buildLayout (QWidget *widget, Orientation orientation)
{
    QLayout *layout = 0;

    if (orientation == Orient_Horizontal)
        layout = new QHBoxLayout();
    else
        layout = new QVBoxLayout();

    layout->setContentsMargins (0, 0, 0, 0);

    QString name = widget->objectName();

    if (!name.isEmpty())
    {
        QLabel *label = new QLabel (name, parent);
        label->setBuddy (widget);
        layout->addWidget (label);
    }

    layout->addWidget (widget);

    return layout;
}


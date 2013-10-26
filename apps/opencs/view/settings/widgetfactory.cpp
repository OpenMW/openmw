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


CSVSettings::WidgetFactory::WidgetFactory (QSortFilterProxyModel *model, QWidget *parent)
    : mParent (parent), mSourceModel (model)
{}

QLayout *CSVSettings::WidgetFactory::build (QWidget *widget, const QString &name, Orientation orientation)
{
    //OSX styling
#ifdef Q_OS_MAC
    widget->setStyle(new PlastiqueStyle);
#endif

    widget->setObjectName (name);

    buildMapper (widget, buildModel (widget));
    return buildLayout(widget, name, orientation);
/*
    qDebug() << "building widget based on section: " << objectName() << "; records: " << mFilterProxy->rowCount();

    qDebug() << "record value: " << mFilterProxy->data(mFilterProxy->index(0,2,QModelIndex()), Qt::DisplayRole).toString();
  */
}

QSortFilterProxyModel *CSVSettings::WidgetFactory::buildModel(QWidget *widget)
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
    filter->setFilterFixedString (widget->objectName());
    filter->setDynamicSortFilter (true);
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

//setting construction...  goes where?
/*
CSMSettings::Setting *CSVSettings::AbstractWidget::buildSetting (QSortFilterProxyModel *settingModel)
{
    CSMSettings::Setting *setting = new CSMSettings::Setting (objectName(), "", "", this);

    for (int i = 0; i < settingModel->columnCount(); ++i)
    {
        QModelIndex index = settingModel->index(0,i, QModelIndex());
        setting->setItem (i, settingModel->data(index));
    }

    return setting;
}
*/

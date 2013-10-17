#include <QLayout>
#include <QLabel>
#include <QSortFilterProxyModel>
#include <QDataWidgetMapper>

#include "abstractwidget.hpp"
#include "../../model/settings/setting.hpp"

#include <QDebug>

CSVSettings::AbstractWidget::AbstractWidget (const QString &name, QLayout *layout, QWidget *parent)
    : QObject (parent), mLayout (layout), mWidget (0), mFilterProxy (0), mDataAdapter (0)
{
    setObjectName (name);
}

void CSVSettings::AbstractWidget::setModel (QSortFilterProxyModel *model)
{
    if (!model)
        return;

    mDataAdapter = new QDataWidgetMapper (parent());
    mFilterProxy = new QSortFilterProxyModel (parent());

    mFilterProxy->setSourceModel (model);
    mFilterProxy->setFilterKeyColumn (0);
    mFilterProxy->setFilterFixedString (objectName());
    mFilterProxy->setDynamicSortFilter (true);

    qDebug() << "building widget based on section: " << objectName() << "; records: " << mFilterProxy->rowCount();

    qDebug() << "record value: " << mFilterProxy->data(mFilterProxy->index(0,2,QModelIndex()), Qt::DisplayRole).toString();
    mDataAdapter->setModel (mFilterProxy);
}

void CSVSettings::AbstractWidget::setWidget(QWidget *widget, int column)
{
    mWidget = widget;

    if (mDataAdapter)
    {
        mDataAdapter->addMapping (mWidget, column);
        mDataAdapter->toFirst();
    }
}

void CSVSettings::AbstractWidget::build (const QString &caption)
{
    if (!caption.isEmpty())
    {
        QLabel *label = new QLabel (caption + "_cap", &dynamic_cast<QWidget &>( *parent()));
        label->setBuddy (mWidget);
        mLayout->addWidget (label);
    }

    mLayout->setContentsMargins(0, 0, 0, 0);
    mLayout->addWidget (mWidget);
    //mLayout->setAlignment (mWidget, getAlignment (align));
}

void CSVSettings::AbstractWidget::createLayout (Orientation direction)
{
    if (direction == Orient_Vertical)
        mLayout = new QVBoxLayout ();
    else
        mLayout = new QHBoxLayout ();
}

QFlags<Qt::AlignmentFlag> CSVSettings::AbstractWidget::getAlignment (CSVSettings::Alignment flag)
{
    return QFlags<Qt::AlignmentFlag>(static_cast<int>(flag));
}

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

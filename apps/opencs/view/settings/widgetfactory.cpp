#include <QSortFilterProxyModel>
#include <QCheckBox>
#include <QComboBox>
#include <QRadioButton>
#include <QListWidget>
#include <QSpinBox>
#include <QLineEdit>
#include <QLabel>

#include <QHBoxLayout>
#include <QVBoxLayout>

#include "widgetfactory.hpp"

CSVSettings::WidgetFactory::WidgetFactory (QLayout *layout,
                                                     QSortFilterProxyModel *model, QWidget *parent)
    : mLayout (layout), mParent (parent), mSourceModel (model)
{}

CSVSettings::SettingWidget *CSVSettings::WidgetFactory::createWidget(const QString &caption, QWidget *widget)
{
    SettingWidget *setting_widget = new SettingWidget (widget, mSourceModel, mParent);


    //mLayout->setAlignment (mWidget, getAlignment (align));


}

CSVSettings::SettingWidget::SettingWidget(QWidget *widget, QSortFilterProxyModel *model, QWidget *parent)
    : mWidget (widget), mFilterProxy (new QSortFilterProxyModel (parent)), mAdapter (0), QWidget (parent)
{

    buildWidgetModel (model);
    buildWidgetView (caption);

    qDebug() << "building widget based on section: " << mWidget->objectName() << "; records: " << mFilterProxy->rowCount();

    qDebug() << "record value: " << mFilterProxy->data(mFilterProxy->index (0, 2, QModelIndex()), Qt::DisplayRole).toString();
}

void CSVSettings::SettingWidget::buildWidgetView(const QString &caption)
{
    setLayout (new QHBoxLayout());

    if (!caption.isEmpty())
    {
        QLabel *label = new QLabel (caption, this);
        label->setBuddy (mWidget);
        layout()->addWidget (label);
    }

    layout()->setContentsMargins(0, 0, 0, 0);
    layout()->addWidget (mWidget);
}

void CSVSettings::SettingWidget::buildWidgetModel(QSortFilterProxyModel *model)
{
    mFilterProxy = new QSortFilterProxyModel (parent);

    //filters the model using the widget name (name of the setting or setting value)
    mFilterProxy->setSourceModel (model);
    mFilterProxy->setFilterKeyColumn (0);
    mFilterProxy->setFilterFixedString (widget->objectName());
    mFilterProxy->setDynamicSortFilter (true);

    mAdapter = new QDataWidgetMapper(this);
    mAdapter->setModel (mFilterProxy);
    mAdapter->addMapping(mWidgetFactory->createWidget(name), 2);
    mAdapter->toFirst();
}

CSVSettings::CheckBoxFactory::CheckBoxFactory (QLayout *layout, QWidget *parent = 0)
    : TypedWidgetFactory (layout, parent)
{}

QWidget *CSVSettings::CheckBoxFactory::createWidget(const QString &name)
{
    setupWidget (name, new QCheckBox(name, mParent));
}

CSVSettings::ComboBoxFactory::ComboBoxFactory (QLayout *layout, QWidget *parent = 0)
    : TypedWidgetFactory (layout, parent)
{}

QWidget *CSVSettings::ComboBoxFactory::createWidget(const QString &name)
{
    setupWidget (name, new QComboBox(name, mParent));
}

CSVSettings::SpinBoxFactory::SpinBoxFactory (QLayout *layout, QWidget *parent = 0)
    : TypedWidgetFactory (layout, parent)
{}

QWidget *CSVSettings::SpinBoxFactory::createWidget(const QString &name)
{
    setupWidget (name, new QSpinBox(name, mParent));
}

CSVSettings::ListBoxFactory::ListBoxFactory (QLayout *layout, QWidget *parent = 0)
    : TypedWidgetFactory (layout, parent)
{}

QWidget *CSVSettings::ListBoxFactory::createWidget(const QString &name)
{
    setupWidget (name, new QListBox(name, mParent));
}

CSVSettings::RadioButtonFactory::RadioButtonFactory (QLayout *layout, QWidget *parent = 0)
    : TypedWidgetFactory (layout, parent)
{}

QWidget *CSVSettings::RadioButtonFactory::createWidget(const QString &name)
{
    setupWidget (name, new QRadioButton(name, mParent));
}

CSVSettings::LineEditFactory::LineEditFactory (QLayout *layout, QWidget *parent = 0)
    : TypedWidgetFactory (layout, parent)
{}

QWidget *CSVSettings::LineEditFactory::createWidget(const QString &name)
{
    setupWidget (name, new QLineEdit(name, mParent));
}

CSVSettings::ToggleButtonFactory::ToggleButtonFactory (QLayout *layout, QWidget *parent = 0)
    : TypedWidgetFactory (layout, parent)
{}

QWidget *CSVSettings::ToggleButtonFactory::createWidget(const QString &name)
{
    //ToggleButton *widget = new ToggleButton (name, mParent);
    //setupWidget (name, widget);
}

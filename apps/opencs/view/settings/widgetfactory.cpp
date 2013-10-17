#include "widgetfactory.hpp"

CSVSettings::TypedWidgetFactory::TypedWidgetFactory (QLayout *layout,
                                                     QSortFilterProxyModel *model, QWidget *parent = 0)
    : mLayout (layout), mParent (parent), mSourceModel (model)
{}

CSVSettings::SettingWidget *CSVSettings::TypedWidgetFactory::setupWidget(const QString &caption, QWidget *widget)
{
    SettingWidget *setting_widget = new SettingWidget (widget, column, mParent);

    if (!caption.isEmpty())
    {
        QLabel *label = new QLabel (caption + "_cap", mParent);
        label->setBuddy (widget);
        mLayout->addWidget (label);
    }

    mLayout->setContentsMargins(0, 0, 0, 0);
    mLayout->addWidget (widget);
    //mLayout->setAlignment (mWidget, getAlignment (align));
}

CSVSettings::SettingWidget::SettingWidget(QWidget *widget, QSortFilterProxyModel *model, QWidget *parent)
{
    if (!model)
        return;

    QDataWidgetMapper mapper = new QDataWidgetMapper (parent);
    QSortFilterProxyModel filter = new QSortFilterProxyModel (parent);

    filter->setSourceModel (model);
    filter->setFilterKeyColumn (0);
    filter->setFilterFixedString (widget->objectName());
    filter->setDynamicSortFilter (true);

    qDebug() << "building widget based on section: " << widget->objectName() << "; records: " << filter->rowCount();

    qDebug() << "record value: " << filter->data(filter->index (0, 2, QModelIndex()), Qt::DisplayRole).toString();
    mapper->setModel (filter);
    mapper->addMapping (widget, 2);
    mapper->toFirst();
}


CSVSettings::CheckBoxFactory::CheckBoxFactory (QLayout *layout, QWidget *parent = 0)
    : TypedWidgetFactory (layout, parent)
{}

QWidget *CSVSettings::CheckBoxFactory::createWidget(const QString &name)
{
    QCheckBox *widget = new QCheckBox(name, mParent);
    setupWidget (name, widget);
}

CSVSettings::ComboBoxFactory::ComboBoxFactory (QLayout *layout, QWidget *parent = 0)
    : TypedWidgetFactory (layout, parent)
{}

QWidget *CSVSettings::ComboBoxFactory::createWidget(const QString &name)
{
    QComboBox *widget = new QComboBox(name, mParent);
    setupWidget (name, widget);
}

CSVSettings::SpinBoxFactory::SpinBoxFactory (QLayout *layout, QWidget *parent = 0)
    : TypedWidgetFactory (layout, parent)
{}

QWidget *CSVSettings::SpinBoxFactory::createWidget(const QString &name)
{
    QSpinBox *widget = new QSpinBox(name, mParent);
    setupWidget (name, widget);
}

CSVSettings::ListBoxFactory::ListBoxFactory (QLayout *layout, QWidget *parent = 0)
    : TypedWidgetFactory (layout, parent)
{}

QWidget *CSVSettings::ListBoxFactory::createWidget(const QString &name)
{
    QSpinBox *widget = new QListBox(name, mParent);
    setupWidget (name, widget);
}

CSVSettings::Factory::RadioButtonFactory (QLayout *layout, QWidget *parent = 0)
    : TypedWidgetFactory (layout, parent)
{}

QWidget *CSVSettings::RadioButtonFactory::createWidget(const QString &name)
{
    QRadioButton *widget = new QRadioButton(name, mParent);
    setupWidget (name, widget);
}

CSVSettings::LineEditFactory::LineEditFactory (QLayout *layout, QWidget *parent = 0)
    : TypedWidgetFactory (layout, parent)
{}

QWidget *CSVSettings::LineEditFactory::createWidget(const QString &name)
{
    QLineEdit *widget = new QLineEdit(name, mParent);
    setupWidget (name, widget);
}

CSVSettings::ToggleButtonFactory::ToggleButtonFactory (QLayout *layout, QWidget *parent = 0)
    : TypedWidgetFactory (layout, parent)
{}

QWidget *CSVSettings::ToggleButtonFactory::createWidget(const QString &name)
{
    //ToggleButton *widget = new ToggleButton (name, mParent);
    //setupWidget (name, widget);
}

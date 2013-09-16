#include "abstractblock.hpp"

CSVSettings::AbstractBlock::AbstractBlock(QWidget* parent)
    : QObject (parent) //, mBox ( new GroupBox (parent) ), mWidgetParent (parent)
{}

CSVSettings::AbstractBlock::AbstractBlock(bool isVisible, QWidget* parent)
    : QObject (parent) //, mBox ( new GroupBox (isVisible, parent)), mWidgetParent (parent)
{}
/*
QLayout *CSVSettings::AbstractBlock::createLayout (Orientation direction, QWidget* parent)
{
    QLayout *layout = 0;

    if (direction == Orient_Vertical)
        layout = new QVBoxLayout (parent);
    else
        layout = new QHBoxLayout (parent);

    layout->setContentsMargins(0, 0, 0, 0);

    return layout;
}

QGroupBox *CSVSettings::AbstractBlock::getGroupBox()
{
    return mBox;
}

CSVSettings::AbstractWidget *CSVSettings::AbstractBlock::buildWidget (const QString& widgetName, WidgetDef &def,
                                                                        QLayout *layout, bool isConnected) const
{
    AbstractWidget *widg = 0;

    switch (def.type)
    {

    case Widget_RadioButton:
        widg = new SettingWidget<QRadioButton> (layout,
               def.orientation, def.widgetAlignment,
               def.isDefault, 0, def.caption, mBox);
        break;

    case Widget_SpinBox:
        widg =  new SettingWidget<QSpinBox> (layout,
                 def.orientation, def.widgetAlignment,
                 def.isDefault, 0, def.caption, mBox);

        break;

    case Widget_CheckBox:
        widg = new SettingWidget<QCheckBox> (layout,
                 def.orientation, def.widgetAlignment,
                 def.isDefault, 0, def.caption, mBox);

        break;

    case Widget_LineEdit:
        widg = new SettingWidget<QLineEdit> (layout,
                 def.orientation, def.widgetAlignment,
                 def.isDefault, 0, def.caption, mBox);

        break;

    case Widget_ListBox:
        widg = new SettingWidget<QListWidget> (layout,
               def.orientation, def.widgetAlignment,
               def.isDefault,  0, def.caption, mBox);

        break;

    case Widget_ComboBox:
        widg = new SettingWidget<QComboBox> (layout,
             def.orientation, def.widgetAlignment,
             def.isDefault, 0, def.caption, mBox);

        break;

    default:
        break;
    };

    if (!mBox->layout())
        mBox->setLayout(widg->getLayout());

    widg->widget()->setObjectName(widgetName);

    return widg;
}

void CSVSettings::AbstractBlock::setVisible (bool isVisible)
{
    mBox->setBorderVisibility (isVisible);
}

bool CSVSettings::AbstractBlock::isVisible () const
{
    return mBox->borderVisibile();
}

QWidget *CSVSettings::AbstractBlock::getParent() const
{
    return mWidgetParent;
}

void CSVSettings::AbstractBlock::slotSetEnabled(bool value)
{
    return;
}
*/

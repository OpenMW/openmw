#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QAbstractSpinBox>
#include <QAbstractSlider>
#include <QDial>
#include <QSlider>

#include "rangeview.hpp"
#include "spinbox.hpp"
#include "../../model/settings/setting.hpp"
#include "../../model/settings/support.hpp"

CSVSettings::RangeView::RangeView (CSMSettings::Setting *setting,
                                       Page *parent)
    : View (setting, parent), mRangeWidget (0), mRangeType (setting->type())
{

    mRangeWidget = 0;

    if (isMultiValue())
        return;

    switch (mRangeType)
    {
    case CSMSettings::Type_SpinBox:
    case CSMSettings::Type_DoubleSpinBox:
        buildSpinBox (setting);
    break;

    case CSMSettings::Type_Dial:
    case CSMSettings::Type_Slider:
        buildSlider (setting);
    break;

    default:
        break;
    }

    if(mRangeWidget)
    {
        mRangeWidget->setFixedWidth (widgetWidth (setting->widgetWidth()));
        mRangeWidget->setObjectName (setting->name());
    }

    addWidget (mRangeWidget);
}

void CSVSettings::RangeView::buildSlider (CSMSettings::Setting *setting)
{
    switch (setting->type())
    {
    case CSMSettings::Type_Slider:
        mRangeWidget = new QSlider (Qt::Horizontal, this);
        mRangeWidget->setProperty ("tickInterval", setting->tickInterval());

        if (setting->ticksAbove())
        {
            if (setting->ticksBelow())
                mRangeWidget->setProperty ("tickPosition", QSlider::TicksBothSides);
            else
                mRangeWidget->setProperty ("tickPosition", QSlider::TicksAbove);
        }
        else if (setting->ticksBelow())
            mRangeWidget->setProperty ("tickPosition", QSlider::TicksBelow);
        else
            mRangeWidget->setProperty ("tickPosition", QSlider::NoTicks);

    break;

    case CSMSettings::Type_Dial:
        mRangeWidget = new QDial (this);
        mRangeWidget->setProperty ("wrapping", setting->wrapping());
        mRangeWidget->setProperty ("notchesVisible",
                            (setting->ticksAbove() || setting->ticksBelow()));
    break;

    default:
        break;
    }

    if(mRangeWidget)
    {
        mRangeWidget->setProperty ("minimum", setting->minimum());
        mRangeWidget->setProperty ("maximum", setting->maximum());
        mRangeWidget->setProperty ("tracking", false);
        mRangeWidget->setProperty ("singleStep", setting->singleStep());

        connect (mRangeWidget, SIGNAL (valueChanged (int)),
                 this, SLOT (slotUpdateView (int)));
    }
}

void CSVSettings::RangeView::buildSpinBox (CSMSettings::Setting *setting)
{
    SpinBox *sb = 0;

    switch (setting->type())
    {
    case CSMSettings::Type_SpinBox:

        sb = new SpinBox (this);

        if (!setting->declaredValues().isEmpty())
            sb->setValueList (setting->declaredValues());

        mRangeWidget = sb;

        connect (mRangeWidget, SIGNAL (valueChanged (int)),
                this, SLOT (slotUpdateView (int)));
    break;

    case CSMSettings::Type_DoubleSpinBox:
        mRangeWidget = new QDoubleSpinBox (this);

        connect (mRangeWidget, SIGNAL (valueChanged (double)),
                this, SLOT (slotUpdateView (double)));
    break;

    default:
        return;
    }

    //min / max values are set automatically in AlphaSpinBox
    if (setting->declaredValues().isEmpty())
    {
        mRangeWidget->setProperty ("minimum", setting->minimum());
        mRangeWidget->setProperty ("maximum", setting->maximum());
        mRangeWidget->setProperty ("singleStep", setting->singleStep());
    }

    mRangeWidget->setProperty ("prefix", setting->prefix());
    mRangeWidget->setProperty ("suffix", setting->suffix());
    mRangeWidget->setProperty ("wrapping", setting->wrapping());
    dynamic_cast<QAbstractSpinBox *> (mRangeWidget)->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

    if(setting->type()  == CSMSettings::Type_SpinBox && setting->declaredValues().isEmpty())
        dynamic_cast<QSpinBox *> (mRangeWidget)->setValue (setting->defaultValues().at(0).toInt());
}

void CSVSettings::RangeView::slotUpdateView (int value)
{
    QString textValue = "";
    QStringList list;

    switch (mRangeType)
    {
    case CSMSettings::Type_SpinBox:
        list = static_cast <SpinBox *> (mRangeWidget)->valueList();
        if (!list.isEmpty())
            textValue = list.at(value);
    break;

    default:
    break;
    }

    if (textValue.isEmpty())
        textValue = QVariant (value).toString();

    setSelectedValue (textValue, false);

    View::updateView();
}

void CSVSettings::RangeView::slotUpdateView (double value)
{
    setSelectedValue (QVariant(value).toString(), false);

    View::updateView();
}

void CSVSettings::RangeView::updateView (bool signalUpdate) const
{
     QString value;

    if (!selectedValues().isEmpty())
        value = selectedValues().at(0);

    switch (mRangeType)
    {
    case CSMSettings::Type_SpinBox:
        static_cast <SpinBox *> (mRangeWidget)->setValue (value);
    break;

    case CSMSettings::Type_DoubleSpinBox:
        static_cast <QDoubleSpinBox *> (mRangeWidget)->setValue (value.toDouble());
    break;

    case CSMSettings::Type_Slider:
    case CSMSettings::Type_Dial:
        mRangeWidget->setProperty ("value", value.toInt());
        mRangeWidget->setProperty ("sliderPosition", value.toInt());
    break;

    default:
    break;

    }

    View::updateView (signalUpdate);
}

CSVSettings::RangeView *CSVSettings::RangeViewFactory::createView
                                        (CSMSettings::Setting *setting,
                                         Page *parent)
{
    return new RangeView (setting, parent);
}

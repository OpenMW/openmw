#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGroupBox>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QAbstractSpinBox>

#include "rangeview.hpp"
#include "spinbox.hpp"
#include "../../model/settings/setting.hpp"
#include "../../model/settings/support.hpp"

CSVSettings::RangeView::RangeView (CSMSettings::Setting *setting,
                                       Page *parent)
    : mRangeWidget (0), mRangeType (setting->type()), View (setting, parent)
{

    mRangeWidget = 0;

    if (isMultiValue())
        return;

    buildSpinBox (setting);

    mRangeWidget->setFixedWidth (widgetWidth (setting->widgetWidth()));
    mRangeWidget->setObjectName (setting->name());

    addWidget (mRangeWidget);
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
    break;
    }

    //min / max values are set automatically in AlphaSpinBox
    if (setting->declaredValues().isEmpty())
    {
        mRangeWidget->setProperty ("minimum", setting->minimum());
        mRangeWidget->setProperty ("maximum", setting->maximum());
        mRangeWidget->setProperty ("singleStep", setting->singleStep());
        mRangeWidget->setProperty ("specialValueText",
                                  setting->specialValueText());
    }

    mRangeWidget->setProperty ("prefix", setting->prefix());
    mRangeWidget->setProperty ("suffix", setting->suffix());
    mRangeWidget->setProperty ("wrapping", setting->wrapping());

}

void CSVSettings::RangeView::slotUpdateView (int value)
{
    QString textValue = "";

    if (mRangeType == CSMSettings::Type_SpinBox)
    {
        QStringList list =
                static_cast <SpinBox *> (mRangeWidget)->valueList();
        if (!list.isEmpty())
            textValue = list.at(value);
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

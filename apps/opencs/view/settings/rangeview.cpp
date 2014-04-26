#include <QHBoxLayout>
#include <QVBoxLayout>

#include <QCheckBox>
#include <QRadioButton>
#include <QGroupBox>

#include <QAbstractButton>

#include "rangeview.hpp"
#include "../../model/settings/setting.hpp"

CSVSettings::RangeView::RangeView (CSMSettings::Setting *setting,
                                       Page *parent)
    : View (setting, parent)
{
    foreach (const QString &value, setting->declaredValues())
    {
        QAbstractButton *button = 0;

        if (isMultiValue())
            button = new QCheckBox (value, this);
        else
            button = new QRadioButton (value, this);

        connect (button, SIGNAL (clicked (bool)),
                this, SLOT (slotToggled (bool)));

        button->setObjectName (value);

        addWidget (button);

        mButtons[value] = button;
    }
}

void CSVSettings::RangeView::slotToggled (bool state)
{
    //test only for true to avoid multiple selection updates with radiobuttons
    if (!isMultiValue() && !state)
        return;

    QStringList values;

    foreach (QString key, mButtons.keys())
    {
        if (mButtons.value(key)->isChecked())
            values.append (key);
    }
    setSelectedValues (values, false);

    View::updateView();
}

void CSVSettings::RangeView::updateView (bool signalUpdate) const
{

    QStringList values = selectedValues();

    foreach (const QString &buttonName, mButtons.keys())
    {
        QAbstractButton *button = mButtons[buttonName];

        //if the value is not found in the list, the widget is checked false
        bool buttonValue = values.contains(buttonName);

        //skip if the butotn value will not change
        if (button->isChecked() == buttonValue)
            continue;

        //disable autoexclusive if it's enabled and we're setting
        //the button value to false
        bool switchExclusive = (!buttonValue && button->autoExclusive());

        if (switchExclusive)
            button->setAutoExclusive (false);

        button->setChecked (buttonValue);

        if (switchExclusive)
            button->setAutoExclusive(true);
    }
    View::updateView (signalUpdate);
}

CSVSettings::RangeView *CSVSettings::RangeViewFactory::createView
                                        (CSMSettings::Setting *setting,
                                         Page *parent)
{
    return new RangeView (setting, parent);
}

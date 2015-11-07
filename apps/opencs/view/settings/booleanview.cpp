#include <QHBoxLayout>
#include <QVBoxLayout>

#include <QCheckBox>
#include <QRadioButton>
#include <QGroupBox>

#include <QAbstractButton>

#include "booleanview.hpp"
#include "../../model/settings/setting.hpp"

CSVSettings::BooleanView::BooleanView (CSMSettings::Setting *setting,
                                       Page *parent)
    : View (setting, parent), mType(setting->type())
{
    foreach (const QString &value, setting->declaredValues())
    {
        QAbstractButton *button = 0;

        switch (mType)
        {
        case CSMSettings::Type_CheckBox: {
                if(mButtons.empty()) // show only one for checkboxes
                {
                    button = new QCheckBox (value, this);
                    button->setChecked (setting->defaultValues().at(0) == "true" ? true : false);

                    // special visual treatment option for checkboxes
                    if(setting->specialValueText() != "") {
                        Frame::setTitle("");
                        button->setText(setting->specialValueText());
                    }
                }
            }
        break;

        case CSMSettings::Type_RadioButton:
            button = new QRadioButton (value, this);
        break;

        default:
        break;
        }

        if(button && (mType != CSMSettings::Type_CheckBox || mButtons.empty()))
        {
            connect (button, SIGNAL (clicked (bool)),
                    this, SLOT (slotToggled (bool)));

            button->setObjectName (value);

            addWidget (button);

            mButtons[value] = button;
        }
    }
}

void CSVSettings::BooleanView::slotToggled (bool state)
{
    //test only for true to avoid multiple selection updates with radiobuttons
    if (!isMultiValue() && !state)
        return;

    QStringList values;

    foreach (QString key, mButtons.keys())
    {
        // checkbox values are true/false unlike radio buttons
        if(mType == CSMSettings::Type_CheckBox)
            values.append(mButtons.value(key)->isChecked() ? "true" : "false");
        else
        {
            if (mButtons.value(key)->isChecked())
                values.append (key);
        }
    }
    setSelectedValues (values, false);

    View::updateView();
}

void CSVSettings::BooleanView::updateView (bool signalUpdate) const
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

CSVSettings::BooleanView *CSVSettings::BooleanViewFactory::createView
                                        (CSMSettings::Setting *setting,
                                         Page *parent)
{
    return new BooleanView (setting, parent);
}

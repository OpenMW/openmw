#include <QHBoxLayout>
#include <QVBoxLayout>

#include <QCheckBox>
#include <QRadioButton>
#include <QGroupBox>

#include <QAbstractButton>

#include "booleanview.hpp"
#include "../../model/settings/setting.hpp"
#include "../../model/settings/selector.hpp"
#include "settingbox.hpp"

#include <QDebug>

CSVSettings::BooleanView::BooleanView (CSMSettings::Setting *setting,
                                       Page *parent)
    : View (setting, parent)
{
    foreach (const QString &value, setting->declaredValues())
    {
        QAbstractButton *button = 0;

        if (setting->isMultiValue())
            button = new QCheckBox (value, this);
        else
            button = new QRadioButton (value, this);

        connect (button, SIGNAL (clicked (bool)),
                this, SLOT (slotToggled (bool)));

        viewFrame()->addWidget (button);
        mButtons[value] = button;
    }
}

void CSVSettings::BooleanView::slotToggled (bool state)
{
    //test only for true to avoid multiple selection updates with radiobuttons
    if (!setting()->isMultiValue() && !state)
        return;

    QStringList values;

    foreach (QString key, mButtons.keys())
    {
        if (mButtons.value(key)->isChecked())
            values.append (key);
    }
    qDebug () << "slotToggled::values = " << values;
    selector()->setViewSelection (values);
}

void CSVSettings::BooleanView::slotUpdateView (const QStringList values)
{
qDebug() << objectName() << "boolean view update" << values;

    foreach (const QString &buttonName, mButtons.keys())
    {

        QAbstractButton *button = mButtons[buttonName];

        //if the value is not found in the list, the widget is checked false
        bool buttonValue = values.contains(buttonName);

        //skip if the butotn value will not change
        if (button->isChecked() == buttonValue)
            continue;

qDebug() << objectName() << buttonName <<  " = " << buttonValue;

        //disable autoexclusive if it's enabled and we're setting
        //the button value to false
        bool switchExclusive = (!buttonValue && button->autoExclusive());

        if (switchExclusive)
            button->setAutoExclusive (false);

qDebug () << objectName() << buttonName << " setting check state";

        button->setChecked (buttonValue);

        if (switchExclusive)
            button->setAutoExclusive(true);
    }
qDebug() << objectName() << "::boleanView::slotUpdateView() complete";
}

CSVSettings::BooleanView *CSVSettings::BooleanViewFactory::createView
                                        (CSMSettings::Setting *setting,
                                         Page *parent)
{
    return new BooleanView (setting, parent);
}

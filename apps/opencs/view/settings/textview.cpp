#include <QTextEdit>
#include <QLineEdit>
#include <QGroupBox>
#include <QLayout>
#include <QApplication>

#include "textview.hpp"
#include "../../model/settings/setting.hpp"
#include "settingbox.hpp"

#include <QDebug>
CSVSettings::TextView::TextView(CSMSettings::Setting *setting,
                                Page *parent)
    : View (setting, parent)

{
    if (setting->isMultiLine())
        mTextWidget = new QTextEdit ("", this);
    else
        mTextWidget = new QLineEdit ("", this);

    if (setting->widgetWidth() > 0)
    {
        QString widthToken;
        widthToken.fill('P', setting->widgetWidth());
        QFontMetrics fm (QApplication::font());
        mTextWidget->setFixedWidth (fm.width (widthToken));
    }

    connect (mTextWidget, SIGNAL (textEdited (QString)),
           this, SLOT (slotTextEdited (QString)));

    viewFrame()->addWidget (mTextWidget,
                            setting->viewRow(), setting->viewColumn());
}

void CSVSettings::TextView::slotTextEdited (QString value)
{
    QStringList values = value.split (setting()->delimiter(),
                                      QString::SkipEmptyParts);

    QStringList returnValues;

    foreach (const QString &splitValue, values)
        returnValues.append (splitValue.trimmed());

    setSelectedValues (returnValues, false);

    View::updateView();
}

void CSVSettings::TextView::updateView() const
{
    QStringList values = selectedValues();

    if (values.size() == 0)
        return;

    QString valueString;

    for (int i = 0; i < values.size() - 1; i++)
        valueString += (values.at(i) + setting()->delimiter());

    valueString += values.back();

    if ( !(mTextWidget->property("text").toString() == valueString))
        mTextWidget->setProperty ("text", valueString);

    View::updateView();
}

CSVSettings::TextView *CSVSettings::TextViewFactory::createView
                                    (CSMSettings::Setting *setting,
                                     Page *parent)
{
    return new TextView (setting, parent);
}


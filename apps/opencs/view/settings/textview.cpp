#include <QTextEdit>
#include <QLineEdit>
#include <QGroupBox>
#include <QLayout>
#include <QApplication>

#include "textview.hpp"
#include "../../model/settings/setting.hpp"
#include "settingbox.hpp"
#include "../../model/settings/selector.hpp"

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
    qDebug () << objectName() << "TextView::slotTextEdited() value = " << value;

    QStringList values = value.split (setting()->delimiter(),
                                      QString::SkipEmptyParts);

    selector()->setData(values);
    selector()->setViewSelection (values);
}

void CSVSettings::TextView::slotUpdateView (const QStringList values)
{
    if (values.size() == 0)
        return;

    QString valueString;

    for (int i = 0; i < values.size() - 1; i++)
        valueString += (values.at(i) + setting()->delimiter());

    valueString += values.back();

    mTextWidget->setProperty ("text", valueString);
}

CSVSettings::TextView *CSVSettings::TextViewFactory::createView
                                    (CSMSettings::Setting *setting,
                                     Page *parent)
{
    return new TextView (setting, parent);
}


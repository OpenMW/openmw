#include <QTextEdit>
#include <QLineEdit>
#include <QGroupBox>
#include <QLayout>
#include <QDataWidgetMapper>
#include <QDebug>
#include <QApplication>

#include "textview.hpp"
#include "../../model/settings/setting.hpp"
#include "../../model/settings/textadapter.hpp"
#include "settingbox.hpp"

CSVSettings::TextView::TextView(QAbstractItemModel *textAdapter,
                                const CSMSettings::Setting *setting,
                                QWidget *parent)
    : View (textAdapter, setting, parent)

{
    buildView (setting);
    buildModel (setting);
}

void CSVSettings::TextView::buildView(const CSMSettings::Setting *setting)
{
    setObjectName (setting->settingName + "_view");

    QWidget *widget = 0;

    if (setting->isMultiLineText)
        mTextWidget = new QTextEdit ("", this);
    else
        mTextWidget = new QLineEdit ("", this);

    if (setting->widgetWidth > 0)
    {
        QString widthToken;
        widthToken.fill('P', setting->widgetWidth);
        QFontMetrics fm (QApplication::font());
        mTextWidget->setFixedWidth (fm.width (widthToken));
    }

    mTextWidget->setObjectName (setting->settingName + "_textWidget");

    viewFrame()->addWidget (mTextWidget, setting->viewRow, setting->viewColumn);
}

void CSVSettings::TextView::buildModel (const CSMSettings::Setting *setting)
{
    QByteArray propName;

    if (setting->isMultiLineText)
        propName = "plainText";
    else
        propName = "text";

    QDataWidgetMapper *mapper =
            new QDataWidgetMapper (mTextWidget);

    mapper->setObjectName
                (setting->settingName + '.' +
                 mTextWidget->objectName() + "_mapper");

    mapper->setModel (model());
    mapper->addMapping (mTextWidget, CSMSettings::Setting_Value, propName);
    mapper->setCurrentIndex(0);
}

CSVSettings::TextView *CSVSettings::TextViewFactory::createView
    (CSMSettings::DefinitionModel &model, const CSMSettings::Setting *setting)
{
    QWidget *widgParent = static_cast<QWidget *>(parent());

    CSMSettings::TextAdapter *adapter =
            new CSMSettings::TextAdapter (model, setting, widgParent);

    return new TextView (adapter, setting, widgParent);
}


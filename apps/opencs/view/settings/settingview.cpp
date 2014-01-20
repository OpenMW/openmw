#include <QSortFilterProxyModel>
#include <QDataWidgetMapper>
#include <QHBoxLayout>

#include "settingview.hpp"
#include "settingviewcomponent.hpp"
#include "settingfactory.hpp"
#include "apps/opencs/model/settings/setting.hpp"
#include "../../model/settings/iwidgetadapter.hpp"
#include "../../model/settings/binarywidgetadapter.hpp"

#include "support.hpp"

#include <QDebug>

CSVSettings::SettingView::SettingView (const CSMSettings::Setting *setting, QWidget *parent) :
    QGroupBox(parent)
{
    build(setting);
}

void CSVSettings::SettingView::build(const CSMSettings::Setting *setting)
{
    QString settingName = setting->name();

    setupView(setting->isHorizontal());
    setObjectName (settingName);
    setTitle (settingName);

    CSMSettings::IWidgetAdapter *adapter = 0;

    switch (setting->widgetType())
    {
    case Widget_RadioButton:
    case Widget_CheckBox:

        adapter = new CSMSettings::BinaryWidgetAdapter (setting->section(),
                                                        settingName,
                                                        this);
    break;

    default:

       adapter = 0;
    break;

    }

    if (setting->valueList().size() > 0)
    {
       foreach (const QString &text, setting->valueList())
           buildComponent(text, setting->widgetType(), adapter);
    }
    else
        buildComponent(setting->name(), setting->widgetType(), adapter);
}

void CSVSettings::SettingView::buildComponent(const QString &text,
                                              WidgetType widgetType,
                                              CSMSettings::IWidgetAdapter *adapter)
{
    SettingViewComponent *svComponent = new SettingViewComponent (text,
                                                                  widgetType);
    layout()->addWidget(svComponent->widget());
}

//view layout code


const QString CSVSettings::SettingView::INVISIBLE_BOX_STYLE =
        QString::fromUtf8("QSettingBox { border: 0px; padding 0px; margin: 0px;}");

void CSVSettings::SettingView::setupView(bool isHorizontal)
{
    setFlat (true);

    if (isHorizontal)
        setLayout (new QHBoxLayout);
    else
        setLayout (new QVBoxLayout);
}

void CSVSettings::SettingView::setTitle (const QString &title)
{
    //if (borderVisibile() )
    {
        QGroupBox::setTitle (title);
        QGroupBox::setCheckable(false);
        //set minimum width to accommodate title, if needed
        //1.5 multiplier to account for bold title.
        QFontMetrics fm (font());
        int minWidth = fm.width(QGroupBox::title());
        setMinimumWidth (minWidth * 1.5);
    }
}

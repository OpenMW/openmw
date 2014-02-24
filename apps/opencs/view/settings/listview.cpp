#include "listview.hpp"
#include "../../model/settings/adapter.hpp"
#include "../../model/settings/setting.hpp"
#include "settingbox.hpp"

#include <QListView>
#include <QComboBox>
#include <QApplication>
#include <QStandardItemModel>
CSVSettings::ListView::ListView(const CSMSettings::Setting &setting,
                                QWidget *parent)
    : View(setting, parent)
{/*
    setObjectName (setting->settingName + "_view");

    QWidget *widget = 0;

    if (setting->isMultiLineText) {
        QListView *widget = new QListView (this);
        mListWidget = widget;
        widget->setModel (listModel);
    }
    else {
        QComboBox *widget = new QComboBox (this);
        mListWidget = widget;
        widget->setModel (listModel);
    }

    if (setting->widgetWidth > 0)
    {
        QString widthToken;
        widthToken.fill('P', setting->widgetWidth);
        QFontMetrics fm (QApplication::font());
        mListWidget->setFixedWidth (fm.width (widthToken));
    }

    mListWidget->setObjectName (setting->settingName + "_listWidget");

    viewFrame()->addWidget (mListWidget, setting->viewRow, setting->viewColumn);
    build(setting);*/
}

void CSVSettings::ListView::build(const CSMSettings::Setting *setting)
{

}

CSVSettings::ListView *CSVSettings::ListViewFactory::createView
                                        (const CSMSettings::Setting &setting)
{/*
    //create a generic adapter just for the setting filter it
    //maintains internally.
    CSMSettings::Adapter *adapter = new CSMSettings::Adapter(model,
        setting->pageName,setting->settingName, setting->isMultiValue, this);
*/
  //  return new ListView(adapter->filter(), setting, this);
}

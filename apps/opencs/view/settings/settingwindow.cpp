#include <QApplication>

#include "../../model/settings/setting.hpp"
#include "settingwindow.hpp"
#include "page.hpp"

#include <QDebug>

CSVSettings::SettingWindow::SettingWindow(QWidget *parent)
    : QMainWindow(parent)
{}

void CSVSettings::SettingWindow::createPages()
{
    CSMSettings::SettingPageMap pageMap = mModel->settingPageMap();

    foreach (const QString &pageName, pageMap.keys())
    {
        qDebug() << "creating pages with settings:";
        CSMSettings::SettingList list = pageMap.value (pageName);
        foreach (CSMSettings::Setting *setting, list)
        {
            qDebug() << setting->name() << "; " << setting->declaredValues() << "; " << setting->definedValues();
        }

        mPages.append (new Page (pageName, list, this));
    }
}

CSMSettings::Selector *CSVSettings::SettingWindow::selector
                        (const QString &pageName, const QString &settingName)
{
    return mModel->selector(pageName, settingName);
}

void CSVSettings::SettingWindow::closeEvent (QCloseEvent *event)
{
    QApplication::focusWidget()->clearFocus();
}

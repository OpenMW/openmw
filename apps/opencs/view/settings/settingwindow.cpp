#include <QApplication>

#include "../../model/settings/setting.hpp"
#include "../../model/settings/connector.hpp"
#include "settingwindow.hpp"
#include "page.hpp"
#include "view.hpp"

#include <QDebug>

CSVSettings::SettingWindow::SettingWindow(QWidget *parent)
    : QMainWindow(parent)
{}

void CSVSettings::SettingWindow::createPages()
{
    CSMSettings::SettingPageMap pageMap = mModel->settingPageMap();

    CSMSettings::SettingList connectedSettings;

    foreach (const QString &pageName, pageMap.keys())
    {
        CSMSettings::SettingList pageSettings = pageMap.value (pageName);
        mPages.append (new Page (pageName, pageSettings, this));

        for (int i = 0; i < pageSettings.size(); i++)
        {
            CSMSettings::Setting *setting = pageSettings.at(i);

            if (!setting->proxyLists().isEmpty())
                connectedSettings.append (setting);
        }
    }

    if (!connectedSettings.isEmpty())
        createConnections(connectedSettings);
}

void CSVSettings::SettingWindow::createConnections
                                        (const CSMSettings::SettingList &list)
{
    foreach (const CSMSettings::Setting *setting, list)
    {
        const CSMSettings::ProxySettingPairs &proxyLists = setting->proxyLists();

        View *masterView = findView (setting->page(), setting->name());

        CSMSettings::Connector *connector =
                                new CSMSettings::Connector (masterView, this);

        connect (masterView,    SIGNAL  (viewUpdated()),
                 connector,     SLOT    (slotUpdateSlaves())
                 );

        foreach (const CSMSettings::ProxySettingPair &pair, proxyLists)
        {
            CSMSettings::StringPair names = pair.first;

            View *slaveView = findView (names.first, names.second);

            if (!slaveView)
            {
                qWarning () << "Unable to create connection for view "
                            << names.first << '.' << names.second;
                continue;
            }

            connector->addSlaveView (slaveView, pair.second);

            connect (slaveView, SIGNAL  (viewUpdated()),
                    connector,  SLOT    (slotUpdateMaster()));
        }
    }
}

CSVSettings::View *CSVSettings::SettingWindow::findView
                            (const QString &pageName, const QString &setting)
{
    foreach (const Page *page, mPages)
    {
        if (page->objectName() == pageName)
            return page->findView (pageName, setting);
    }
    return 0;
}

void CSVSettings::SettingWindow::closeEvent (QCloseEvent *event)
{
    QApplication::focusWidget()->clearFocus();
}

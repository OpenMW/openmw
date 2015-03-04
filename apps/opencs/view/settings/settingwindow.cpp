#include <QApplication>
#include <QDebug>

#include "../../model/settings/setting.hpp"
#include "../../model/settings/connector.hpp"
#include "../../model/settings/usersettings.hpp"
#include "settingwindow.hpp"
#include "page.hpp"
#include "view.hpp"

CSVSettings::SettingWindow::SettingWindow(QWidget *parent)
    : QMainWindow(parent), mModel(NULL)
{}

void CSVSettings::SettingWindow::createPages()
{
    CSMSettings::SettingPageMap pageMap = mModel->settingPageMap();

    QList <CSMSettings::Setting *> connectedSettings;

    foreach (const QString &pageName, pageMap.keys())
    {
        QList <CSMSettings::Setting *> pageSettings = pageMap.value (pageName).second;

        mPages.append (new Page (pageName, pageSettings, this, pageMap.value (pageName).first));

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
                                    (const QList <CSMSettings::Setting *> &list)
{
    foreach (const CSMSettings::Setting *setting, list)
    {
        View *masterView = findView (setting->page(), setting->name());

        CSMSettings::Connector *connector =
                                new CSMSettings::Connector (masterView, this);

        connect (masterView,
                 SIGNAL (viewUpdated(const QString &, const QStringList &)),
                 connector,
                 SLOT (slotUpdateSlaves())
                 );

        const CSMSettings::ProxyValueMap &proxyMap = setting->proxyLists();

        foreach (const QString &key, proxyMap.keys())
        {
            QStringList keyPair = key.split('/');

            if (keyPair.size() != 2)
                continue;

            View *slaveView = findView (keyPair.at(0), keyPair.at(1));

            if (!slaveView)
            {
                qWarning () << "Unable to create connection for view "
                            << key;
                continue;
            }

            QList <QStringList> proxyList = proxyMap.value (key);
            connector->addSlaveView (slaveView, proxyList);

            connect (slaveView,
                     SIGNAL (viewUpdated(const QString &, const QStringList &)),
                    connector,
                     SLOT (slotUpdateMaster()));
        }
    }
}

void CSVSettings::SettingWindow::setViewValues()
{
    //iterate each page and view, setting their definitions
    //if they exist in the model
    foreach (const Page *page, mPages)
    {
        foreach (const View *view, page->views())
        {
            //testing beforehand prevents overwriting a proxy setting
            if (!mModel->hasSettingDefinitions (view->viewKey()))
                continue;

            QStringList defs = mModel->definitions (view->viewKey());

            view->setSelectedValues(defs);
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

void CSVSettings::SettingWindow::saveSettings()
{
    //setting the definition in the model automatically syncs with the file
    foreach (const Page *page, mPages)
    {
        foreach (const View *view, page->views())
        {
            if (!view->serializable())
                continue;

            mModel->setDefinitions (view->viewKey(), view->selectedValues());
        }
    }

    mModel->saveDefinitions();
}


#include "page.hpp"
#include "settingbox.hpp"
#include "view.hpp"
#include "booleanview.hpp"
#include "textview.hpp"
#include "listview.hpp"

#include "../../model/settings/setting.hpp"
#include "../../model/settings/settingmanager.hpp"
#include "../../model/settings/connector.hpp"
#include "settingwindow.hpp"

#include <QDebug>

QMap <CSVSettings::ViewType, CSVSettings::IViewFactory *>
                                            CSVSettings::Page::mViewFactories;

CSVSettings::Page::Page(const QString &pageName,
                        CSMSettings::SettingList settingList,
                        SettingWindow *parent) :
    mSettingList (settingList), mParent(parent)
{
    setObjectName (pageName);

    if (mViewFactories.size() == 0)
        buildFactories();

    setupPage ();
    setupViews (settingList);
}

void CSVSettings::Page::setupPage ()
{
    mBox = new SettingBox (false, "", this);

    SettingLayout *layout = new SettingLayout();
    layout->addWidget (mBox, 0, 0);

    setLayout (layout);
}

void CSVSettings::Page::setupViews
                            (CSMSettings::SettingList &settingList)
{
    foreach (CSMSettings::Setting *setting, settingList)
        addView (setting);
}

const CSMSettings::Setting *CSVSettings::Page::findSetting (const QString &settingName)
{
    qDebug() << objectName() ;
             qDebug()<< "::Page::findSetting " << settingName;
             qDebug() << mSettingList.size();
    foreach (const CSMSettings::Setting *setting, mSettingList)
    {
        if (setting->name() == settingName)
            return setting;
    }
    return 0;
}
/*
void CSVSettings::Page::buildProxy (CSVSettings::View *view)
{
    const CSMSettings::Setting *setting = findSetting (view->objectName());

    const CSMSettings::ProxySettingPairs &proxyLists = setting->proxyLists();

    CSMSettings::Connector *connector =
                                new CSMSettings::Connector(view, this);
    qDebug() << objectName() << "::Page::buildProxy() connecting " << view->objectName() << " to connector::slotUpdateSlaves";
    connect (view, SIGNAL (viewUpdated()),
             connector, SLOT (slotUpdateSlaves()));

    foreach (const CSMSettings::ProxySettingPair &psPair, proxyLists)
    {
        CSMSettings::StringPair names = psPair.first;

        View *proxyView = findView (names.first, names.second);

        if (!proxyView)
        {
            qWarning () << "Unable to create connection for view "
                        << names.first << '.' << names.second;
            continue;
        }
        connector->addSlaveView (proxyView, psPair.second);

        connect (proxyView, SIGNAL (viewUpdated()),
                connector, SLOT (slotUpdateMaster()));
    }

        qDebug() << "----------------------proxySelector construction complete-----------------";
}
*/
void CSVSettings::Page::addView (CSMSettings::Setting *setting)
{
    qDebug() << objectName() << "::Page::addView() " << setting->name();

    if (setting->viewType() == ViewType_Undefined)
        return;

    qDebug() << objectName() << "::Page::addView() factory";
    View *view = mViewFactories[setting->viewType()]->createView
                                                    (setting, this);

    if (!view)
        return;

    qDebug() << objectName() << "::Page::addView() appending view " << view->objectName();

    mViews.append (view);

    mBox->addWidget (view->viewFrame(),
                     setting->viewRow(),
                     setting->viewColumn());
}

CSVSettings::View *CSVSettings::Page::findView (const QString &page,
                                                const QString &setting) const
{
    if (page == objectName())
    {
        for (int i = 0; i < mViews.size(); i++)
        {
            View *view = mViews.at(i);
            if (view->setting()->page() == page)
                if (view->setting()->name() == setting)
                    return view;
        }
        return 0;
    }

    return mParent->findView (page, setting);
}

void CSVSettings::Page::buildFactories()
{
    mViewFactories[ViewType_Boolean] = new BooleanViewFactory (this);
    mViewFactories[ViewType_Text] = new TextViewFactory (this);
    mViewFactories[ViewType_List] = new ListViewFactory (this);
}

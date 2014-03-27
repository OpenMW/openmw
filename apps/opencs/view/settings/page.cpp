#include "page.hpp"
#include "settingbox.hpp"
#include "view.hpp"
#include "booleanview.hpp"
#include "textview.hpp"
#include "listview.hpp"

#include "../../model/settings/selector.hpp"
#include "../../model/settings/setting.hpp"
#include "../../model/settings/settingmanager.hpp"
#include "settingwindow.hpp"

#include <QDebug>

QMap <CSVSettings::ViewType, CSVSettings::IViewFactory *>
                                            CSVSettings::Page::mViewFactories;

CSVSettings::Page::Page(const QString &pageName,
                        CSMSettings::SettingList &settingList,
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

void CSVSettings::Page::buildProxy (CSMSettings::Setting *setting,
                                    CSMSettings::Selector *selector)
{
    qDebug() << "building proxy for setting " << setting->name();
    const CSMSettings::ProxyLists &proxyLists = setting->proxyLists();

    int idx = 0;
    selector->setProxyIndex(idx++);

    //iterate each setting item's map of proxylists to master key values
    //outermost loop retreives setting name and value map and adds the
    //column for the setting's proxy values to be inserted
    foreach (const CSMSettings::ProxySettingPair *settingItem, proxyLists)
    {
        //adds a blank column to the model
        selector->addModelColumn(QStringList());

        CSMSettings::StringPair *proxyNames = settingItem->first;
        CSMSettings::ProxyValueList *valueMap = settingItem->second;

        CSMSettings::Selector *proxySelector = mParent->selector
                                    (proxyNames->first, proxyNames->second);
        qDebug() << "buildProxy() proxySelector created";
        if (!proxySelector)
            continue;
        qDebug () <<" build proxy adding column values";

        //iterates each mapping of proxy value list to master value and inserts
        //the proxy value list in the appropriate cell in the table
        foreach (const CSMSettings::StringListPair *mapPair, *valueMap)
        {
            int row = setting->declaredValues().indexOf (mapPair->first);
            selector->setData(row, idx, mapPair->second);
        }

        qDebug() << "buildProxy() column values added";

        proxySelector->setProxyIndex (idx++);
        proxySelector->setProxySelectionModel (selector->selectionModel());

        qDebug() << "buildProxy() selector indexed and selectionModel set";

        connect (selector,
            SIGNAL (proxyUpdate (int, CSMSettings::StandardItemList)),
            proxySelector,
            SLOT (slotUpdateSlaveByProxy (int, CSMSettings::StandardItemList)));

        connect (proxySelector,
            SIGNAL (proxyUpdate (int, CSMSettings::StandardItemList)),
            selector,
            SLOT (slotUpdateMasterByProxy (int, CSMSettings::StandardItemList)));

        //proxySelector->refresh();
        qDebug() << "----------------------proxySelector construction complete-----------------";
    }
}

void CSVSettings::Page::addView (CSMSettings::Setting *setting)
{

    if (setting->viewType() == ViewType_Undefined)
        return;

    View *view = mViewFactories[setting->viewType()]->createView
                                                    (setting, this);

    if (!view)
        return;

    mViews.append (view);

    mBox->addWidget (view->viewFrame(),
                     setting->viewRow(),
                     setting->viewColumn());
}

void CSVSettings::Page::buildFactories()
{
    mViewFactories[ViewType_Boolean] = new BooleanViewFactory (this);
    mViewFactories[ViewType_Text] = new TextViewFactory (this);
    mViewFactories[ViewType_List] = new ListViewFactory (this);
}

CSMSettings::Selector *CSVSettings::Page::selector (const QString &settingName)
{
    return mParent->selector (objectName(), settingName);
}

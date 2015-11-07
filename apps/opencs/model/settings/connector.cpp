#include "connector.hpp"
#include "../../view/settings/view.hpp"
#include "../../view/settings/page.hpp"

CSMSettings::Connector::Connector(CSVSettings::View *master,
                                            QObject *parent)
    : QObject(parent), mMasterView (master)
{}

void CSMSettings::Connector::addSlaveView (CSVSettings::View *view,
                                        QList <QStringList> &masterProxyValues)
{
    mSlaveViews.append (view);

    mProxyListMap[view->viewKey()].append (masterProxyValues);
}

QList <QStringList> CSMSettings::Connector::getSlaveViewValues() const
{
    QList <QStringList> list;

    foreach (const CSVSettings::View *view, mSlaveViews)
        list.append (view->selectedValues());

    return list;
}

bool CSMSettings::Connector::proxyListsMatch (
                                        const QList <QStringList> &list1,
                                        const QList <QStringList> &list2) const
{
    bool success = true;

    for (int i = 0; i < list1.size(); i++)
    {
        success = stringListsMatch (list1.at(i), list2.at(i));

        if (!success)
            break;
    }
    return success;
}

void CSMSettings::Connector::slotUpdateMaster() const
{
    //list of the current values for each slave.
    QList <QStringList> slaveValueList = getSlaveViewValues();

    int masterColumn = -1;

    /*
    * A row in the master view is one of the values in the
    * master view's data model.  This corresponds directly to the number of
    * values in a proxy list contained in the ProxyListMap member.
    * Thus, we iterate each "column" in the master proxy list
    * (one for each vlaue in the master.  Each column represents
    * one master value's corresponding list of slave values.  We examine
    * each master value's list, comparing it to the current slave value list,
    * stopping when we find a match using proxyListsMatch().
    *
    * If no match is found, clear the master view's value
    */

    for (int i = 0; i < mMasterView->rowCount(); i++)
    {
        QList <QStringList> proxyValueList;

        foreach (const QString &settingKey, mProxyListMap.keys())
        {
            // append the proxy value list stored in the i'th column
            // for each setting key.  A setting key is the id of the setting
            // in page.name format.
            proxyValueList.append (mProxyListMap.value(settingKey).at(i));
        }

        if (proxyListsMatch (slaveValueList, proxyValueList))
        {
            masterColumn = i;
            break;
        }
    }

    QString masterValue = mMasterView->value (masterColumn);

    mMasterView->setSelectedValue (masterValue);
}

void CSMSettings::Connector::slotUpdateSlaves() const
{
    int row = mMasterView->currentIndex();

    if (row == -1)
        return;

    //iterate the proxy lists for the chosen master index
    //and pass the list to each slave for updating
    for (int i = 0; i < mSlaveViews.size(); i++)
    {
        QList <QStringList> proxyList =
                            mProxyListMap.value(mSlaveViews.at(i)->viewKey());

        mSlaveViews.at(i)->setSelectedValues (proxyList.at(row));
    }
}

bool CSMSettings::Connector::stringListsMatch (
                                                const QStringList &list1,
                                                const QStringList &list2) const
{
    //returns a "sloppy" match, verifying that each list contains all the same
    //items, though not necessarily in the same order.

    if (list1.size() != list2.size())
        return false;

    QStringList tempList(list2);

    //iterate each value in the list, removing one occurrence of the value in
    //the other list.  If no corresponding value is found, test fails
    foreach (const QString &value, list1)
    {
        if (!tempList.contains(value))
            return false;

        tempList.removeOne(value);
    }
    return true;
}

#include "connector.hpp"
#include "../../view/settings/view.hpp"

CSMSettings::Connector::Connector(CSVSettings::View *master,
                                            QObject *parent)
    : mMasterView (master), QObject(parent)
{}

void CSMSettings::Connector::addSlaveView (CSVSettings::View *view,
                                    const StringListPairs &masterProxyValues)
{
    mSlaveViews.append (ViewIndexPair (mSlaveViews.size(), view));

    //append the proxy values (pair.second) to the list for the associated
    //master key (pair.first)
    foreach (const StringListPair &masterProxyPair, masterProxyValues)
        mProxyMap[masterProxyPair.first].append (masterProxyPair.second);
}

QList <QStringList> CSMSettings::Connector::getSlaveViewValues() const
{
    QList <QStringList> list;

    foreach (const ViewIndexPair &pair, mSlaveViews)
        list.append (pair.second->selectedValues());

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

CSVSettings::View *CSMSettings::Connector::slaveView (int index) const
{
    foreach (const ViewIndexPair &pair, mSlaveViews)
    {
        if (pair.first == index)
            return pair.second;
    }
    return 0;
}

void CSMSettings::Connector::slotUpdateMaster() const
{
    QString masterKey = "";

    //check this list against ecah of the options in the proxy value map
    foreach (const QString &key, mProxyMap.keys())
    {
        if (proxyListsMatch (getSlaveViewValues(), mProxyMap.value(key)))
        {
            masterKey = key;
            break;
        }
    }
    mMasterView->setSelectedValues(QStringList() << masterKey);
}

void CSMSettings::Connector::slotUpdateSlaves() const
{
    QStringList masterValues = mMasterView->selectedValues();

    if (masterValues.isEmpty())
        return;

    QList <QStringList> proxyLists = mProxyMap.value(masterValues.at(0));

    //iterate the proxy lists for the chosen master index
    //and pass the list to each slave for updating
    for (int i = 0; i < proxyLists.size(); i++)
        slaveView(i)->setSelectedValues (proxyLists.at(i));
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

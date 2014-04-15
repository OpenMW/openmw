#include "connector.hpp"
#include "../../view/settings/view.hpp"
#include "../../view/settings/page.hpp"
#include <QDebug>

CSMSettings::Connector::Connector(CSVSettings::View *master,
                                            QObject *parent)
    : mMasterView (master), QObject(parent)
{}

void CSMSettings::Connector::addSlaveView (CSVSettings::View *view,
                                        QList <QStringList> &masterProxyValues)
{
    mSlaveViews.append (ViewIndexPair (mSlaveViews.size(), view));

    mProxyListMap[view->viewKey()].append (masterProxyValues);
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
    qDebug() << "Connector::slotUpdateMaster()";

    //list of the current values for each slave.
    QList <QStringList> slaveValues = getSlaveViewValues();

    qDebug() << "Connector::slotUpdateMaster() slave values = " << slaveValues;

    int masterColumn = -1;
/*
    //check this list against each of the options in the proxy value map
    for (int i = 0; i < mSlaveViews.size(); i++)
    {
        QString slaveValue = slaveValues.at(i);
        QList <QStringList> proxyValues =
                                mProxyListMap.value (slaveView(i)->viewKey());
        qDebug() << "Connector::slotUpdateMaster() " << i << ": proxy values = " << proxyValues;

        if (masterColumn == -1)
            masterColumn = proxyValues.indexOf (slaveValue);
        else
            if (masterColumn != proxyValues.indexOf (slaveValue))
                break;
    }*/
    mMasterView->setSelectedValue ("");
}

void CSMSettings::Connector::slotUpdateSlaves() const
{
    qDebug () << "Connector::slotUpdateSlaves() ";

    int row = mMasterView->currentIndex();

    if (row == -1)
        return;

    //iterate the proxy lists for the chosen master index
    //and pass the list to each slave for updating
    for (int i = 0; i < mSlaveViews.size(); i++)
    {
        QList <QStringList> proxyList =
                                mProxyListMap.value(slaveView(i)->viewKey());

        qDebug() << "Connector::slotUpdateSlaves() " << i << ':' << proxyList;

        slaveView(i)->setSelectedValues (proxyList.at(row));
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

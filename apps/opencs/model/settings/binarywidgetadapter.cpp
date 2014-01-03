#include <QStringList>

#include "settingmodel.hpp"
#include "binarywidgetadapter.hpp"
#include "usersettings.hpp"
#include "sectionfilter.hpp"

#include <QDebug>
CSMSettings::BinaryWidgetAdapter::BinaryWidgetAdapter(
                                                  const QString &sectionName,
                                                  const QString &settingName,
                                                  QObject *parent) :
    mSettingName(settingName), mSectionName(sectionName),
    mSingleValueMode(false), QAbstractItemModel(parent)
{
    mSettingFilter = new QSortFilterProxyModel(this);

    mSettingFilter->setSourceModel(CSMSettings::UserSettings
                                   ::instance()
                                   .settingModel());
    mSettingFilter->setFilterKeyColumn(4);
    mSettingFilter->setFilterRegExp(sectionName + "." + settingName);

    QModelIndex sourceIndex = mSettingFilter->index(0, 3);
    mValueList = mSettingFilter->data(sourceIndex).toStringList();

    //create a list of QString pairs which represent the setting values and
    //whether or not they are set (true / false)
    foreach (const QString &listValue, mValueList)
    {
        QPair<QString, QBool *> settingPair(listValue, new QBool(false));

        for (int i = 0; i < mSettingFilter->rowCount(); i++)
        {
            QString modelValue = mSettingFilter->data(mSettingFilter->index(i, 2, QModelIndex())).toString();

            if (modelValue == listValue)
            {
                settingPair.second = new QBool(true);
                break;
            }
        }
        mSettings.append(settingPair);
    }

    connect (mSettingFilter, SIGNAL(dataChanged(const QModelIndex &, const QModelIndex &)),
             this, SLOT(slotDataChanged(const QModelIndex &, const QModelIndex &)));

    connect (mSettingFilter,
             SIGNAL(layoutChanged()),
             this, SLOT(slotUpdateData()));
}

void CSMSettings::BinaryWidgetAdapter::slotDataChanged(const QModelIndex &topLeft, const QModelIndex &botRight)
{
    qDebug() << "received dataChanged()!";
}

void CSMSettings::BinaryWidgetAdapter::slotUpdateData()
{
    QString objId = "BinaryWidgetAdapter." + objectName() + "::slotUpdateData()";
    qDebug() << objId << " data: ";

    for (int i = 0; i < mSettings.count(); i++)
    {
        QPair <QString, QBool *> data = mSettings.at(i);

        QString settingValue = data.first;

        bool settingFound = false;

        for (int j = 0; j < mSettingFilter->rowCount(); j++)
        {
            QModelIndex idx = mSettingFilter->index(j, 2, QModelIndex());

            settingFound = (mSettingFilter->data(idx).toString() == settingValue);

            qDebug() << objId << "index: " << i << "; setting found? " << settingFound;
            if (settingFound)
            {
                if ((*data.second) == QBool(false))
                    data.second = new QBool(true);
                break;
            }
        }

        if (!settingFound)
        {
            if (*data.second == QBool(true))
                data.second = new QBool (false);
        }

        qDebug() << objId << " replacing setting with " << data.first << " = " << *data.second;
        mSettings.replace(i, data);
        QModelIndex idx = index(i, 1, QModelIndex());
        emit dataChanged (idx, idx);
    }

    for (int i = 0; i < mSettings.count(); i++)
    {
        qDebug() << "\t" << mSettings.at(i).first << " = " << *(mSettings.at(i).second);
    }

    //emit dataChanged(index(0,0,QModelIndex()), index(rowCount() - 1, 4, QModelIndex()));
}

bool CSMSettings::BinaryWidgetAdapter::insertItem(const QString &item)
{
    //if the item isn't found in the local model, abort
    bool success = false;

    for (int i = 0; i< mSettings.size(); i++)
    {
        success = (mSettings.at(i).first == item);

        if (success)
            break;
    }

    if (!success)
        return false;

    //if the item already exists in the source model, abort
    if (sourceModelIndex(item).isValid())
        return false;

    CSMSettings::UserSettings::instance().settingModel()->defineSetting(mSettingName, mSectionName, item);

    return true;
}

bool CSMSettings::BinaryWidgetAdapter::removeItem(const QString &item)
{
    while (sourceModelIndex(item).isValid())
        mSettingFilter->removeRow(sourceModelIndex(item).row());

    return true;
}

QVariant CSMSettings::BinaryWidgetAdapter::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    int row = index.row();

    if (row < 0 || row >= mSettings.size())
        return QVariant();

    switch (role)
    {
    case Qt::EditRole:
    case Qt::DisplayRole:

        switch (index.column())
        {
        case 0: // setting value
            return mSettings.at(row).first;
        break;

        case 1: // value state (true / false)
            return QVariant(*(mSettings.at(row).second));
        break;
        }

    default:
    break;
    }

    return QVariant();
}

int CSMSettings::BinaryWidgetAdapter::rowCount(const QModelIndex &parent) const
{
    return mSettings.size();
}

int CSMSettings::BinaryWidgetAdapter::columnCount(const QModelIndex &parent) const
{
    return 2;
}

bool CSMSettings::BinaryWidgetAdapter::setData(const QModelIndex &index,
                                                const QVariant &value, int role)
{
    if (!index.isValid())
        return false;

    int row = index.row();

    if (row < 0 || row >= rowCount())
        return false;

    //QVariant val = QVariant(value);
    const QBool retVal = QBool(value.toBool() == true);
    *(mSettings.at(row).second) = retVal;
    QString item = mSettings.at(row).first;

    bool success = false;

    if (mSingleValueMode)
        success = setSingleValue(item);
    else
        success = setMultiValue(value.toBool(), item);

    if (!success)
        return false;

    emit dataChanged(index, index);

    return true;
}

bool CSMSettings::BinaryWidgetAdapter::setMultiValue (bool value, const QString &item)
{
    QModelIndex sourceIndex = sourceModelIndex(item);

    //add to source model
    if (value)
    {
        if (sourceIndex.isValid())
            return false;

        insertItem(item);
    }
    else
    {
        if (!sourceIndex.isValid())
            return false;

        removeItem(item);
    }

    return true;
}

bool CSMSettings::BinaryWidgetAdapter::setSingleValue (const QString &item)
{
    QPair<QString, QBool *> settingPair;
    //search for any values with a true state in the sub model
    foreach (settingPair, mSettings)
    {
        if (settingPair.first == item)
            continue;

        if (*(settingPair.second) == QBool(true))
        {
            removeItem(settingPair.first);
            *(settingPair.second) = QBool(false);
        }
    }
    insertItem(item);

    return true;
}

Qt::ItemFlags CSMSettings::BinaryWidgetAdapter::flags(const QModelIndex &index) const
{
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable;
    if (!index.isValid())
        return Qt::NoItemFlags;

    Qt::ItemFlags defaultFlags = Qt::ItemIsSelectable | Qt::ItemIsEnabled;

    if (index.column() == 1)
        return defaultFlags | Qt::ItemIsEditable;

    return defaultFlags;
}

QModelIndex CSMSettings::BinaryWidgetAdapter::index(int row, int column, const QModelIndex &parent) const
{
    if ((row >= 0 && row < rowCount()) && ( column >= 0 && column < columnCount()))
        return createIndex (row, column);

    return QModelIndex();
}

QModelIndex CSMSettings::BinaryWidgetAdapter::sourceModelIndex(const QString &item) const
{
    for (int i = 0; i < mSettingFilter->rowCount(); i++)
    {
        QModelIndex sourceIndex = mSettingFilter->index(i, 2);

        if (mSettingFilter->data(sourceIndex).toString() == item)
            return sourceIndex;
    }
    return QModelIndex();
}

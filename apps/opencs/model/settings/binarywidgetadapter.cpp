#include <QStringList>

#include "settingmodel.hpp"
#include "binarywidgetadapter.hpp"
#include "usersettings.hpp"
#include "sectionfilter.hpp"

#include <QDebug>
CSMSettings::BinaryWidgetAdapter::BinaryWidgetAdapter(SectionFilter *filter,
                                                  const QString &settingName,
                                                  QObject *parent) :
    mSettingName(settingName), mFilter(filter), QAbstractItemModel(parent)
{
    mSettingFilter.setSourceModel(filter);
    mSettingFilter.setFilterKeyColumn(0);
    mSettingFilter.setFilterRegExp(settingName);
    mSettingFilter.setDynamicSortFilter(true);

    QModelIndex sourceIndex = mSettingFilter.index(0, 3);
    mValueList = mSettingFilter.data(sourceIndex).toStringList();

    //create a list of QString pairs which represent the setting values and
    //whether or not they are set (true / false)
    foreach (const QString &listValue, mValueList)
    {
        QPair<QString, QBool *> settingPair(listValue, new QBool(false));

        for (int i = 0; i < mSettingFilter.rowCount(); i++)
        {
            QString modelValue = mSettingFilter.data(mSettingFilter.index(i, 2, QModelIndex())).toString();

            if (modelValue == listValue)
            {
                settingPair.second = new QBool(true);
                break;
            }
        }
        mSettings.append(settingPair);
    }

    connect (&mSettingFilter, SIGNAL(dataChanged(QModelIndex &, QModelIndex &)),
             this, SIGNAL(dataChanged(QModelIndex &, QModelIndex &)));

    connect (&mSettingFilter, SIGNAL(layoutChanged()), this, SIGNAL(layoutChanged()));
}

bool CSMSettings::BinaryWidgetAdapter::insertItem(const QString &item)
{
    qDebug() << "inserting item: " << item;
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

    mFilter->createSetting(mSettingName, item, mValueList);

    return true;
}

bool CSMSettings::BinaryWidgetAdapter::removeItem(const QString &item)
{
    qDebug() << "removing item: " << item;
    while (sourceModelIndex(item).isValid())
        removeRow(sourceModelIndex(item).row());

    return true;
}

QVariant CSMSettings::BinaryWidgetAdapter::data(const QModelIndex &index, int role) const
{
    qDebug () << "BinaryWidgetAdapter::data()";

    if (!index.isValid())
        return QVariant();

    int row = index.row();
    qDebug () << "BinaryWidgetAdapter::data() row";
    if (row < 0 || row >= mSettings.size())
        return QVariant();

    qDebug() << "BinaryWidgetAdapter::data index row: " << row;

    switch (role)
    {
    case Qt::EditRole:
    case Qt::DisplayRole:

        switch (index.column())
        {
        case 0: // setting value

            qDebug() << "setting value column: " << mSettings.at(row).first;
            return mSettings.at(row).first;
        break;

        case 1: // value state (true / false)
            qDebug() << "setting: " << mSettings.at(row).first;
            qDebug() << "setting state column: " << *(mSettings.at(row).second);
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
    qDebug () << "BinaryWidgetAdapter::setData()";

    if (!index.isValid())
        return false;

    int row = index.row();

    if (row < 0 || row >= rowCount())
        return false;

    qDebug () << "BinaryWidgetAdapter::setData() row " << row << "; col: " << index.column() << "; value: " << value.toBool();

    QVariant val = QVariant(value);
    const QBool retVal = QBool(val.toBool() == true);
    *(mSettings.at(row).second) = retVal;
    QString item = mSettings.at(row).first;

    QModelIndex sourceIndex = sourceModelIndex(item);

    //add to source model
    if (value.toBool())
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

    emit dataChanged(index, index);

    qDebug() << "SETTING DUMP";
    for (int i = 0; i< mSettingFilter.rowCount(); i++)
    {
        QModelIndex sIndx2 = mSettingFilter.index(i, 0);
        QModelIndex sIndx = mSettingFilter.index(i, 2);

        QString stngNam = mSettingFilter.data(sIndx2).toString();
        QString stngVal = mSettingFilter.data(sIndx).toString();

        qDebug() << "setting: " << stngNam << " = " << stngVal;

    }
    return true;
}

Qt::ItemFlags CSMSettings::BinaryWidgetAdapter::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;

    Qt::ItemFlags defaultFlags = Qt::ItemIsSelectable | Qt::ItemIsEnabled;

    if (index.column() == 1)
        return defaultFlags | Qt::ItemIsEditable;

    return defaultFlags;
}

QModelIndex CSMSettings::BinaryWidgetAdapter::index(int row, int column, const QModelIndex &parent) const
{
    qDebug() << "BinaryWidgetAdapter::index row: " << row << "; column: " << column;
    if ((row >= 0 && row < rowCount()) && ( column >= 0 && column < columnCount()))
        return createIndex (row, column);

    return QModelIndex();
}

QModelIndex CSMSettings::BinaryWidgetAdapter::sourceModelIndex(const QString &item) const
{
    for (int i = 0; i < mSettingFilter.rowCount(); i++)
    {
        QModelIndex sourceIndex = mSettingFilter.index(i, 0);
        if (mSettingFilter.data(sourceIndex).toString() == item)
            return sourceIndex;
    }
    return QModelIndex();
}

#include "declarationmodel.hpp"

#include "setting.hpp"

#include <QDebug>


CSMSettings::DeclarationModel::DeclarationModel(QObject *parent) :
    QAbstractItemModel(parent)
{}

int CSMSettings::DeclarationModel::columnCount(const QModelIndex &parent) const
{
    return 7;
}

int CSMSettings::DeclarationModel::rowCount(const QModelIndex &parent) const
{
    return mDeclarations.size();
}

QVariant CSMSettings::DeclarationModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    Setting *setting = mDeclarations.at(index.row()).second.second;

    SettingColumn columnEnum = static_cast<SettingColumn>(index.column());

    switch (role)
    {
    case Qt::EditRole:
    case Qt::DisplayRole:

        switch (columnEnum)
        {
        case Setting_Name:
            return setting->settingName;
        break;

        case Setting_Page:
            return setting->pageName;
        break;

        case Setting_DefaultValue:
            return setting->defaultValue;
        break;

        case Setting_ValueList:
            return setting->valueList;
        break;

        case Setting_ViewType:
            return static_cast<int> (setting->viewType);
        break;

        case Setting_ValueCapacity:
            return setting->isMultiValue;
        break;

        case Setting_Orientation:
            return setting->isHorizontal;
        break;

        default:
        break;
        }

    default:
    break;
    }

    return QVariant();
}

Qt::ItemFlags CSMSettings::DeclarationModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;

    return Qt::ItemIsSelectable | Qt::ItemIsEnabled;
}

CSMSettings::Setting *CSMSettings::DeclarationModel::singleBool (
        const QString &pageName, const QString &settingName,
        const QStringList &valueList, const QString &defaultValue,
        bool isHorizontal)
{
    return declareSetting
            (CSVSettings::ViewType_Boolean, false, pageName, settingName,
             valueList, defaultValue, "", isHorizontal);
}

CSMSettings::Setting *CSMSettings::DeclarationModel::multiBool (
        const QString &pageName, const QString &settingName,
        const QStringList &valueList, const QString &defaultValue,
        bool isHorizontal)
{
    return  declareSetting
            (CSVSettings::ViewType_Boolean, true, pageName, settingName,
             valueList, defaultValue, "", isHorizontal);
}

CSMSettings::Setting *CSMSettings::DeclarationModel::singleList (
        const QString &pageName, const QString &settingName,
        const QStringList &valueList, const QString &defaultValue,
        bool isHorizontal)
{
    return declareSetting
            (CSVSettings::ViewType_List, false, pageName, settingName,
             valueList, defaultValue, "", isHorizontal);
}

CSMSettings::Setting *CSMSettings::DeclarationModel::multiList (
        const QString &pageName, const QString &settingName,
        const QStringList &valueList, const QString &defaultValue,
        bool isHorizontal)
{
    return declareSetting
            (CSVSettings::ViewType_List, true, pageName, settingName, valueList,
             defaultValue, "", isHorizontal);
}

CSMSettings::Setting *CSMSettings::DeclarationModel::singleText (
        const QString &pageName, const QString &settingName,
        const QString &defaultValue, const QString &inputMask,
        bool isHorizontal)
{
    return declareSetting
            (CSVSettings::ViewType_Text, false, pageName, settingName,
             QStringList(), defaultValue, inputMask, isHorizontal);
}

CSMSettings::Setting *CSMSettings::DeclarationModel::multiText (
        const QString &pageName, const QString &settingName,
        const QString &defaultValue, const QString &inputMask,
        bool isHorizontal)
{
    return declareSetting
            (CSVSettings::ViewType_Text, true, pageName, settingName,
             QStringList(), defaultValue, inputMask, isHorizontal);
}

CSMSettings::Setting *CSMSettings::DeclarationModel::declareSetting (
        CSVSettings::ViewType viewType, bool isMultiValue,
        const QString &pageName, const QString &settingName,
        const QStringList &valueList, const QString &defaultValue,
        const QString &inputMask, bool isHorizontal)
{
    Setting *setting =
            new Setting(viewType, settingName, pageName);

    setting->defaultValue = defaultValue;
    setting->valueList = valueList;
    setting->inputMask = inputMask;
    setting->isHorizontal = isHorizontal;
    setting->isMultiValue = isMultiValue;

    DeclarationPair decPair;
    decPair.first = settingName;
    decPair.second = setting;

    DeclarationListItem decListItem;
    decListItem.first = pageName;
    decListItem.second = decPair;

    mDeclarations.append(decListItem);

    return setting;
}


QModelIndex CSMSettings::DeclarationModel::index(int row, int column,
                                             const QModelIndex &parent) const
{
    if (!hasIndex(row, column, parent))
        return QModelIndex();

    if ((row >= 0 && row < rowCount()) &&
        (column >= 0 && column < columnCount()))
    {
            return createIndex (row, column);
    }

    return QModelIndex();
}

CSMSettings::Setting *CSMSettings::DeclarationModel::getSetting
                    (const QString &pageName, const QString &settingName) const
{
    foreach (const DeclarationListItem &item, mDeclarations)
    {
        if (item.first == pageName)
        {
            if (item.second.first == settingName)
            {
                qDebug() << "found setting: " << item.second.second->settingName;
                return item.second.second;
            }
        }
    }

    return 0;
}

CSMSettings::Setting *CSMSettings::DeclarationModel::getSetting (int row) const
{
    if (row >= 0 && row < rowCount())
        return mDeclarations.at(row).second.second;

    return 0;
}

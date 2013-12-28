#ifndef SETTINGMODEL_HPP
#define SETTINGMODEL_HPP

#include <QAbstractItemModel>
#include <QSortFilterProxyModel>
#include <QList>

#include "setting.hpp"

namespace CSMSettings
{
    class SettingData : public QObject
    {
        QString mSection;
        QString mValue;
        const QStringList &mValueList;

        Q_OBJECT

    public:
        SettingData (const QString &section, const QString &name,
                     const QStringList &valueList, QObject *parent = 0)
            : mSection (section), mValueList(valueList), QObject(parent)
        { setObjectName(name); }

        void setValue (const QString &value)    { mValue = value; }

        QString value () const                  { return mValue;  }

        QString section() const                 { return mSection; }
        QString name() const                    { return objectName(); }
        const QStringList &valueList() const    { return mValueList; }
    };

    class SettingModel : public QAbstractItemModel
    {

        Q_OBJECT

        //need to put add / remove functionality in user settings class
        //and remove all storage devices from the model.
        QList<SettingData *> mSettings;

    public:

        explicit SettingModel(QObject *parent);

        int rowCount (const QModelIndex &parent = QModelIndex()) const;
        int columnCount (const QModelIndex &parent = QModelIndex()) const;
        QVariant data (const QModelIndex &index, int role) const;

        bool setData (const QModelIndex &index, const QVariant &value, int role = Qt::EditRole);

        bool setDataByName (const QString &sectionName, const QString &settingName, const QVariant &value);
        bool addDataByName (const QString &sectionName, const QString &settingName, const QVariant &value);

        Qt::ItemFlags flags (const QModelIndex &index) const;

        QModelIndex index(int row, int column, const QModelIndex &parent) const;

        QModelIndex parent(const QModelIndex &child) const
        { return QModelIndex(); }

        bool insertRows(int row, int count, const QModelIndex &parent);
        bool removeRows(int row, int count, const QModelIndex &parent);

        SettingData *createSetting (const QString &name, const QString &section,
                                    const QString &value, const QStringList &valueList);

        const SettingData *getSetting (int row) const;
        SettingData *getSetting (const QString &sectionName, const QString &settingName);

    signals:

    public slots:

    };
}
#endif // SETTINGMODEL_HPP

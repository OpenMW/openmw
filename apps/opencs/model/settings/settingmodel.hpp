#ifndef SETTINGMODEL_HPP
#define SETTINGMODEL_HPP

#include <QAbstractItemModel>
#include <QSortFilterProxyModel>
#include <QList>

#include "setting.hpp"

namespace CSMSettings
{
    class SettingModel : public QAbstractItemModel
    {

        Q_OBJECT

        QList<Setting *> mSettings;

    public:

        explicit SettingModel(QObject *parent);

        ~SettingModel();

        int rowCount (const QModelIndex &parent = QModelIndex()) const;
        int columnCount (const QModelIndex &parent = QModelIndex()) const;
        QVariant data (const QModelIndex &index, int role) const;

        bool setData (const QModelIndex &index, const QVariant &value, int role = Qt::EditRole);
        bool setDataByName (const QString &sectionName, const QString &settingName, const QVariant &value);

        Qt::ItemFlags flags (const QModelIndex &index) const;

        QModelIndex index(int row, int column, const QModelIndex &parent) const;

        QModelIndex parent(const QModelIndex &child) const
        { return QModelIndex(); }

        Setting *createSetting (const QString &name, const QString &section = "", const QString &defaultValue = "");
        const Setting *getSetting (int row) const;

    signals:

    public slots:

    };

    class SectionFilter : public QSortFilterProxyModel
    {
        Q_OBJECT

    public:
        explicit SectionFilter (QObject *parent) : QSortFilterProxyModel (parent)
        {}

        const Setting *getSetting (int row) const
        {
            QModelIndex sourceIndex = mapToSource(index(row, 0, QModelIndex()));
            return static_cast<SettingModel *>(sourceModel())->getSetting(sourceIndex.row());
        }
    };
}
#endif // SETTINGMODEL_HPP

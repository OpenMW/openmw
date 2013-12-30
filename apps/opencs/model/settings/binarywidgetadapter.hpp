#ifndef BINARYWIDGETMODEL_HPP
#define BINARYWIDGETMODEL_HPP

#include <QList>
#include <QPair>
#include <QSortFilterProxyModel>
#include "usersettings.hpp"

namespace CSMSettings
{
    class SectionFilter;

    class BinaryWidgetAdapter : public QAbstractItemModel
    {
        Q_OBJECT

        QStringList mValueList;
        QString mSettingName;
        SectionFilter *mFilter;
        QSortFilterProxyModel mSettingFilter;
        QList<QPair<QString, QBool *> > mSettings;

    public:

        explicit BinaryWidgetAdapter(SectionFilter *filter,
                                   const QString &settingName,
                                   QObject *parent = 0);

        int rowCount(const QModelIndex &parent = QModelIndex()) const;
        int columnCount(const QModelIndex &parent = QModelIndex()) const;

        const QStringList &valueList() const    { return mValueList; }
        bool insertItem         (const QString &item);
        bool removeItem         (const QString &item);

        QVariant data           (const QModelIndex &index,
                                 int role = Qt::DisplayRole) const;

        bool setData            (const QModelIndex &index,
                                 const QVariant &value,
                                 int role = Qt::EditRole);

        Qt::ItemFlags flags     (const QModelIndex &index) const;

        QModelIndex index(int row, int column, const QModelIndex &parent) const;

        QModelIndex parent(const QModelIndex &child) const  { return QModelIndex(); }

    private:
        QModelIndex sourceModelIndex (const QString &item) const;
    };
}
#endif // BINARYWIDGETMODEL_HPP

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
        QString mSectionName;
       // SectionFilter *mFilter;
        QSortFilterProxyModel *mSettingFilter;
        QList<QPair<QString, QBool *> > mSettings;
        bool mSingleValueMode;

    public:

        explicit BinaryWidgetAdapter(
                                   const QString &sectionName,
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

        void setSingleValueMode (bool state)                { mSingleValueMode = state; }
        bool singleValueMode () const                       {return mSingleValueMode; }
        QSortFilterProxyModel *filter()         {return mSettingFilter; }

    private:

        QModelIndex sourceModelIndex (const QString &item) const;

        bool setSingleValue(const QString &item);
        bool setMultiValue(bool value, const QString &item);

    private slots:
        void slotUpdateData();
        void slotDataChanged (const QModelIndex &, const QModelIndex &);

    };
}
#endif // BINARYWIDGETMODEL_HPP

#ifndef CSMSETTINGS_ADAPTER_HPP
#define CSMSETTINGS_ADAPTER_HPP

#include <QAbstractItemModel>

#include "../../view/settings/support.hpp"

class QSortFilterProxyModel;
class QStandardItemModel;

namespace CSMSettings
{

    class Adapter : public QAbstractItemModel
    {
        Q_OBJECT

        bool mIsMultiValue;
        QStandardItemModel &mDefModel;
        QSortFilterProxyModel *mSettingFilter;
        QString mPageName;
        QString mSettingName;

    public:

        explicit Adapter (QStandardItemModel &model, const QString &pageName,
                          const QString &settingName, bool isMultiValue,
                          QObject *parent = 0);

        bool isMultiValue () const                  { return mIsMultiValue; }
        QSortFilterProxyModel *filter()             { return mSettingFilter; }

    protected:

        virtual int rowCount
                            (const QModelIndex &parent = QModelIndex()) const;

        virtual int columnCount
                            (const QModelIndex &parent = QModelIndex()) const;

        virtual QVariant data   (const QModelIndex &index,
                                int role = Qt::DisplayRole) const;

        virtual bool setData    (const QModelIndex &index,
                                const QVariant &value,
                                int role = Qt::EditRole);

        virtual QModelIndex index
                        (int row, int column, const QModelIndex &parent) const;

        virtual Qt::ItemFlags flags(const QModelIndex &index) const;

        QModelIndex parent(const QModelIndex &child) const
                                                    { return QModelIndex(); }

        bool insertValue (const QString &value);
        bool removeValue (const QString &value);
        bool validIndex(QModelIndex idx) const;

        const QSortFilterProxyModel *filter() const { return mSettingFilter; }

        QString pageName() const                    { return mPageName; }
        QString settingName() const                 { return mSettingName; }

        QStandardItemModel &defModel() const    { return mDefModel; }

    private:

        QModelIndex valueSourceIndex    (const QString &value,
                                        SettingProperty column) const;

        QSortFilterProxyModel *buildFilter (QAbstractItemModel *model,
                                            SettingProperty column,
                                            const QString &expression);
    private slots:

        virtual void slotLayoutChanged()
        {}

        virtual void slotDataChanged(const QModelIndex &, const QModelIndex &)
        {}
    };
}
#endif // CSMSETTINGS_ADAPTER_HPP

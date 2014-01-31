#ifndef CSMSETTINGS_ADAPTER_HPP
#define CSMSETTINGS_ADAPTER_HPP

#include <QAbstractItemModel>

#include "../../view/settings/support.hpp"

class QSortFilterProxyModel;

namespace CSMSettings
{
    class DefinitionModel;

    class Adapter : public QAbstractItemModel
    {
        Q_OBJECT

        bool mIsMultiValue;
        DefinitionModel &mModel;
        QSortFilterProxyModel *mSettingFilter;
        QString mPageName;
        QString mSettingName;

    public:

        explicit Adapter (DefinitionModel &model, const QString &pageName,
                          const QString &settingName, bool isMultiValue,
                          QObject *parent = 0);

        bool isMultiValue () const             { return mIsMultiValue; }

    protected:

        virtual int rowCount
                            (const QModelIndex &parent = QModelIndex()) const;

        virtual int columnCount
                            (const QModelIndex &parent = QModelIndex()) const;

        virtual QVariant data   (const QModelIndex &index,
                                int role = Qt::DisplayRole) const = 0;

        virtual bool setData    (const QModelIndex &index,
                                const QVariant &value,
                                int role = Qt::EditRole) = 0;

        virtual QModelIndex index
                        (int row, int column, const QModelIndex &parent) const;

        virtual Qt::ItemFlags flags(const QModelIndex &index) const;

        QModelIndex parent(const QModelIndex &child) const
                                                    { return QModelIndex(); }

        bool valueExists (const QString &value);
        bool insertValue (const QString &value);
        bool removeValue (const QString &value);
        bool validIndex(QModelIndex idx) const;

        const QSortFilterProxyModel *filter() const { return mSettingFilter; }
        QSortFilterProxyModel *filter()             { return mSettingFilter; }
        QString pageName() const                    { return mPageName; }
        QString settingName() const                 { return mSettingName; }

        DefinitionModel &model() const    { return mModel; }

    private:

        QModelIndex valueSourceIndex    (const QString &value,
                                        SettingColumn column) const;

        QSortFilterProxyModel *buildFilter (QAbstractItemModel *model,
                                            SettingColumn column,
                                            const QString &expression);
    private slots:

        virtual void slotLayoutChanged() = 0;
        virtual void slotDataChanged(const QModelIndex &, const QModelIndex &) = 0;
    };
}
#endif // CSMVSETTINGS_ADAPTER_HPP

#ifndef CSVSETTINGS_VIEWADAPTER_HPP
#define CSVSETTINGS_VIEWADAPTER_HPP

#include <QAbstractItemModel>

#include "../../view/settings/support.hpp"

class QSortFilterProxyModel;

namespace CSMSettings
{
    class DefinitionModel;

    class ViewAdapter : public QAbstractItemModel
    {
        bool mSingleValueMode;
        DefinitionModel &mModel;
        QSortFilterProxyModel *mSettingFilter;
        QString mPageName;
        QString mSettingName;

    public:

        explicit ViewAdapter (DefinitionModel &model,
                              const QString &pageName,
                              const QString &settingName,
                              QObject *parent = 0);

        void setSingleValueMode (bool state)    { mSingleValueMode = state; }
        bool singleValueMode () const           { return mSingleValueMode; }

    protected:
        bool valueExists (const QString &value, SettingColumn column) const;

        QModelIndex valueSourceIndex (const QString &value,
                               SettingColumn column) const;

        bool insertValue (const QString &value);
        bool removeValue (const QString &value);

        QSortFilterProxyModel *buildFilter (QAbstractItemModel *model,
                                            SettingColumn column,
                                            const QString &expression);

        QSortFilterProxyModel *filter()         { return mSettingFilter; }
        QString pageName() const                { return mPageName; }
        QString settingName() const             { return mSettingName; }

    private:

        QVariant sourceData (int row, int column) const;
    };
}
#endif // CSVSETTINGS_VIEWADAPTER_HPP

#ifndef CSM_WOLRD_IDTABLEPROXYMODEL_H
#define CSM_WOLRD_IDTABLEPROXYMODEL_H

#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <QModelIndex>
#include <QSortFilterProxyModel>
#include <QString>
#include <QTimer>

#include "../prefs/state.hpp"

#include "columns.hpp"

class QObject;

namespace CSMFilter
{
    class Node;
}

namespace CSMWorld
{
    class IdTableBase;

    class IdTableProxyModel : public QSortFilterProxyModel
    {
        Q_OBJECT

        std::shared_ptr<CSMFilter::Node> mFilter;
        std::unique_ptr<QTimer> mFilterTimer;
        std::shared_ptr<CSMFilter::Node> mAwaitingFilter;
        std::map<int, int> mColumnMap; // column ID, column index in this model (or -1)

        // Cache of enum values for enum columns (e.g. Modified, Record Type).
        // Used to speed up comparisons during the sort by such columns.
        typedef std::map<Columns::ColumnId, std::vector<std::pair<int, std::string>>> EnumColumnCache;
        mutable EnumColumnCache mEnumColumnCache;

    protected:
        IdTableBase* mSourceModel;

    private:
        void updateColumnMap();

    public:
        IdTableProxyModel(QObject* parent = nullptr);

        virtual QModelIndex getModelIndex(const std::string& id, int column) const;

        void setSourceModel(QAbstractItemModel* model) override;

        void setFilter(const std::shared_ptr<CSMFilter::Node>& filter);

        void refreshFilter();

    protected:
        bool lessThan(const QModelIndex& left, const QModelIndex& right) const override;

        bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const override;

        QString getRecordId(int sourceRow) const;

    protected slots:

        virtual void sourceRowsInserted(const QModelIndex& parent, int start, int end);

        virtual void sourceRowsRemoved(const QModelIndex& parent, int start, int end);

        virtual void sourceDataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight);

        void timerTimeout();

        void settingChanged(const CSMPrefs::Setting* setting);

    signals:

        void rowAdded(const std::string& id);
    };
}

#endif

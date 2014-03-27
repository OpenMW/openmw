#ifndef CSMSETTINGS_SELECTOR_HPP
#define CSMSETTINGS_SELECTOR_HPP

#include <QObject>
#include <QItemSelection>
#include <QAbstractItemModel>
#include <QStandardItemModel>
#include <QModelIndexList>

class QItemSelectionModel;
class QStandardItem;

namespace CSMSettings
{
    class Setting;

    typedef QList <QStandardItem *> StandardItemList;

    class Selector : public QObject
    {
        Q_OBJECT

        QItemSelectionModel *mSelectionModel;
        QItemSelectionModel *mProxySelectionModel;
        QStandardItemModel *mDataModel;

        bool mSelectAll;
        int mProxyIndex;

    public:
        explicit Selector(const QString &name, const QStringList &list, QObject *parent = 0);

        void dumpDataModel() const;

        void addModelColumn (const QStringList &columnValues);
        void addModelRow ();

        void setSelectAll();
        void selectAll() const;

        //used externally
        void setViewSelection (const QStringList &list) const;

        //used internally
        void setSelection (const QModelIndexList &indices,
                           QItemSelectionModel::SelectionFlags flags,
                           bool emitUpdate = true) const;

        void setData (const QStringList &list);
        void setData (int row, int column, const QStringList &valueList);
        QStringList data (int row, int column) const;

        void setProxyIndex (int value)                  { mProxyIndex = value; }
        void setModel (QStandardItemModel *model);
        void setProxySelectionModel (QItemSelectionModel *model)
                                            { mProxySelectionModel = model; }
        int proxyIndex() const                          { return mProxyIndex; }

        void refresh();
        int rowCount() const;
        int columnCount()  const;

        QItemSelection selection() const;
        QList <int> selectedRows() const;
        QStringList selectionToStringList() const   { return toStringList (selection().indexes()); }

        QItemSelectionModel *selectionModel()       { return mSelectionModel; }
        QStandardItemModel *model()
                    { return dynamic_cast<QStandardItemModel *> (mDataModel); }

    private:

        bool validCoordinate (int row = -1, int column = -1) const;
        QModelIndexList columnIndices (int column) const;
        QStringList data (QModelIndex index) const;
        QModelIndex index (int row = 0, int column = 0) const;
        bool isSelected (const QModelIndex &idx) const;
        QModelIndexList rowIndices (int row) const;
        QModelIndexList matchByColumn (const QStringList &matchValues,
                                       int column = 0) const;

        void select (const QModelIndexList &list,
                     QItemSelectionModel::SelectionFlags flags,
                     bool emitUpdate = true) const;
/*
        void select (const QModelIndex &idx,
                     QItemSelectionModel::SelectionFlags flags,
                     bool emitUpdate = true) const;
*/
        void select (const QItemSelection &selection,
                     QItemSelectionModel::SelectionFlags flags,
                     bool emitUpdate = true) const;

        bool stringListsMatch (const QStringList &list1,
                              const QStringList &list2) const;


        StandardItemList toStandardItemList (const QModelIndexList &list) const;
        StandardItemList toStandardItemList (const QStringList &list) const;

        QStringList toStringList (const QModelIndexList &list) const;

    signals:

        ///Passes list of currently selected items (signal for views)
        void modelUpdate (const QStringList list) const;

        ///Passes the selected item's row (signal from proxied views)
        void proxyUpdate (int column,
                          CSMSettings::StandardItemList values) const;

    public slots:

        ///Emits modelUpdate(QStringList) from model selection changes
        void slotUpdate (const QItemSelection &, const QItemSelection &);

        ///Responds to signals from proxied settings
        void slotUpdateMasterByProxy (int column,
                                      CSMSettings::StandardItemList values);
        void slotUpdateSlaveByProxy (int column,
                                     CSMSettings::StandardItemList values);
    };
}
#endif // CSMSETTINGS_SELECTOR_HPP

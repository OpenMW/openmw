#ifndef CSV_WORLD_UTIL_H
#define CSV_WORLD_UTIL_H

#include <QAbstractTableModel>
#include <QStyledItemDelegate>

class QUndoStack;

namespace CSVWorld
{
    ///< \brief Getting the data out of an editor widget
    ///
    /// Really, Qt? Really?
    class NastyTableModelHack : public QAbstractTableModel
    {
            QAbstractItemModel& mModel;
            QVariant mData;

        public:

            NastyTableModelHack (QAbstractItemModel& model);

            int rowCount (const QModelIndex & parent = QModelIndex()) const;

            int columnCount (const QModelIndex & parent = QModelIndex()) const;

            QVariant data  (const QModelIndex & index, int role = Qt::DisplayRole) const;

            bool setData (const QModelIndex &index, const QVariant &value, int role = Qt::EditRole);

            QVariant getData() const;
    };

    ///< \brief Use commands instead of manipulating the model directly
    class CommandDelegate : public QStyledItemDelegate
    {
            QUndoStack& mUndoStack;
            bool mEditLock;

        public:

            CommandDelegate (QUndoStack& undoStack, QObject *parent);

            void setModelData (QWidget *editor, QAbstractItemModel *model, const QModelIndex& index) const;

            void setEditLock (bool locked);
    };
}

#endif

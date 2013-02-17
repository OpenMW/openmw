#ifndef CSV_WORLD_UTIL_H
#define CSV_WORLD_UTIL_H

#include <map>

#include <QAbstractTableModel>
#include <QStyledItemDelegate>

#include "../../model/world/columnbase.hpp"

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

    class CommandDelegate;

    class CommandDelegateFactory
    {
        public:

            virtual ~CommandDelegateFactory();

            virtual CommandDelegate *makeDelegate (QUndoStack& undoStack, QObject *parent) const = 0;
            ///< The ownership of the returned CommandDelegate is transferred to the caller.
    };

    class CommandDelegateFactoryCollection
    {
            static CommandDelegateFactoryCollection *sThis;
            std::map<CSMWorld::ColumnBase::Display, CommandDelegateFactory *> mFactories;

        private:

            // not implemented
            CommandDelegateFactoryCollection (const CommandDelegateFactoryCollection&);
            CommandDelegateFactoryCollection& operator= (const CommandDelegateFactoryCollection&);

        public:

            CommandDelegateFactoryCollection();

            ~CommandDelegateFactoryCollection();

            void add (CSMWorld::ColumnBase::Display display, CommandDelegateFactory *factory);
            ///< The ownership of \æ factory is transferred to *this.
            ///
            /// This function must not be called more than once per value of \æ display.

            CommandDelegate *makeDelegate (CSMWorld::ColumnBase::Display display, QUndoStack& undoStack,
                QObject *parent) const;
            ///< The ownership of the returned CommandDelegate is transferred to the caller.
            ///
            /// If no factory is registered for \a display, a CommandDelegate will be returned.

            static const CommandDelegateFactoryCollection& get();

    };

    ///< \brief Use commands instead of manipulating the model directly
    class CommandDelegate : public QStyledItemDelegate
    {
            QUndoStack& mUndoStack;
            bool mEditLock;

        protected:

            QUndoStack& getUndoStack() const;

            virtual void setModelDataImp (QWidget *editor, QAbstractItemModel *model,
                const QModelIndex& index) const;

        public:

            CommandDelegate (QUndoStack& undoStack, QObject *parent);

            virtual void setModelData (QWidget *editor, QAbstractItemModel *model,
                const QModelIndex& index) const;

            void setEditLock (bool locked);

            bool isEditLocked() const;
    };
}

#endif

#ifndef CSV_WORLD_UTIL_H
#define CSV_WORLD_UTIL_H

#include <map>

#include <QAbstractTableModel>
#include <QStyledItemDelegate>


#ifndef Q_MOC_RUN
#include "../../model/world/columnbase.hpp"
#include "../../model/doc/document.hpp"
#endif

class QUndoStack;

namespace CSMWorld
{
    class TableMimeData;
    class UniversalId;
    class CommandDispatcher;
}

namespace CSMPrefs
{
    class Setting;
}

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

            virtual CommandDelegate *makeDelegate (CSMWorld::CommandDispatcher *dispatcher,
                CSMDoc::Document& document, QObject *parent)
                const = 0;
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
            ///< The ownership of \a factory is transferred to *this.
            ///
            /// This function must not be called more than once per value of \a display.

            CommandDelegate *makeDelegate (CSMWorld::ColumnBase::Display display,
                CSMWorld::CommandDispatcher *dispatcher, CSMDoc::Document& document,
                QObject *parent) const;
            ///< The ownership of the returned CommandDelegate is transferred to the caller.
            ///
            /// If no factory is registered for \a display, a CommandDelegate will be returned.

            static const CommandDelegateFactoryCollection& get();

    };

    ///< \brief Use commands instead of manipulating the model directly
    class CommandDelegate : public QStyledItemDelegate
    {
            Q_OBJECT

            bool mEditLock;
            CSMWorld::CommandDispatcher *mCommandDispatcher;
            CSMDoc::Document& mDocument;

        protected:

            QUndoStack& getUndoStack() const;

            CSMDoc::Document& getDocument() const;

            CSMWorld::ColumnBase::Display getDisplayTypeFromIndex(const QModelIndex &index) const;

            virtual void setModelDataImp (QWidget *editor, QAbstractItemModel *model,
                const QModelIndex& index) const;

        public:

            /// \param commandDispatcher If CommandDelegate will be only be used on read-only
            /// cells, a 0-pointer can be passed here.
            CommandDelegate (CSMWorld::CommandDispatcher *commandDispatcher, CSMDoc::Document& document, QObject *parent);

            virtual void setModelData (QWidget *editor, QAbstractItemModel *model,
                const QModelIndex& index) const;

            virtual QWidget *createEditor (QWidget *parent,
                                           const QStyleOptionViewItem& option,
                                           const QModelIndex& index) const;

            virtual QWidget *createEditor (QWidget *parent,
                                           const QStyleOptionViewItem& option,
                                           const QModelIndex& index,
                                           CSMWorld::ColumnBase::Display display) const;

            void setEditLock (bool locked);

            bool isEditLocked() const;

            ///< \return Does column require update?

            virtual void setEditorData (QWidget *editor, const QModelIndex& index) const;

            virtual void setEditorData (QWidget *editor, const QModelIndex& index, bool tryDisplay) const;

            /// \attention This is not a slot. For ordering reasons this function needs to be
            /// called manually from the parent object's settingChanged function.
            virtual void settingChanged (const CSMPrefs::Setting *setting);
    };
}

#endif

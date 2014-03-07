#ifndef CSV_WORLD_DIALOGUESUBVIEW_H
#define CSV_WORLD_DIALOGUESUBVIEW_H

#include <map>
#include <memory>

#include <QAbstractItemDelegate>

#include "../doc/subview.hpp"
#include "../../model/world/columnbase.hpp"

class QDataWidgetMapper;
class QSize;
class QEvent;

namespace CSMWorld
{
    class IdTable;
}

namespace CSMDoc
{
    class Document;
}

namespace CSVWorld
{
    class CommandDelegate;

    class DialogueDelegateDispatcher : public QAbstractItemDelegate
    {
        Q_OBJECT
        std::map<int, CommandDelegate*> mDelegates;

        QObject* mParent;

        const CSMWorld::IdTable* mTable; //nor sure if it is needed TODO

        QUndoStack& mUndoStack;

    public:
        DialogueDelegateDispatcher(QObject* parent, CSMWorld::IdTable* table, QUndoStack& undoStack);

        CSVWorld::CommandDelegate* makeDelegate(CSMWorld::ColumnBase::Display display);

        QWidget* makeEditor(CSMWorld::ColumnBase::Display display, const QModelIndex& index);
        ///< will return null if delegate is not present, parent of the widget is same as for dispatcher itself

        virtual void setEditorData (QWidget* editor, const QModelIndex& index) const;

        virtual void setModelData (QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const;

        virtual void paint (QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const;
        ///< does nothing

        virtual QSize sizeHint (const QStyleOptionViewItem& option, const QModelIndex& index) const;
        ///< does nothing

        private slots:
            void editorDataCommited( QWidget * editor );

    };

    class DialogueSubView : public CSVDoc::SubView
    {
            QDataWidgetMapper *mWidgetMapper;
            std::auto_ptr<DialogueDelegateDispatcher> mDispatcher;

        public:

            DialogueSubView (const CSMWorld::UniversalId& id, CSMDoc::Document& document, bool createAndDelete = false);

            virtual void setEditLock (bool locked);
    };
}

#endif
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


    //this can't be nested into the DialogueDelegateDispatcher, because it needs to emit signals
    class DialogueDelegateDispatcherProxy : public QObject
    {
        Q_OBJECT
        class refWrapper
        {
        public:
            refWrapper(const QModelIndex& index);

            const QModelIndex& mIndex;
        };

        QWidget* mEditor;

        CSMWorld::ColumnBase::Display mDisplay;

        std::auto_ptr<refWrapper> mIndexWrapper;
    public:
        DialogueDelegateDispatcherProxy(QWidget* editor, CSMWorld::ColumnBase::Display display);
        QWidget* getEditor() const;

    public slots:
        void editorDataCommited();
        void setIndex(const QModelIndex& index);

    signals:
        void editorDataCommited(QWidget* editor, const QModelIndex& index, CSMWorld::ColumnBase::Display display);
    };

    class DialogueDelegateDispatcher : public QAbstractItemDelegate
    {
        Q_OBJECT
        std::map<int, CommandDelegate*> mDelegates;

        QObject* mParent;

        CSMWorld::IdTable* mTable; //nor sure if it is needed TODO

        QUndoStack& mUndoStack;

        std::vector<DialogueDelegateDispatcherProxy*> mProxys; //once we move to the C++11 we should use unique_ptr

    public:
        DialogueDelegateDispatcher(QObject* parent, CSMWorld::IdTable* table, QUndoStack& undoStack);

        ~DialogueDelegateDispatcher();

        CSVWorld::CommandDelegate* makeDelegate(CSMWorld::ColumnBase::Display display);

        QWidget* makeEditor(CSMWorld::ColumnBase::Display display, const QModelIndex& index);
        ///< will return null if delegate is not present, parent of the widget is same as for dispatcher itself

        virtual void setEditorData (QWidget* editor, const QModelIndex& index) const;

        virtual void setModelData (QWidget* editor, QAbstractItemModel* model, const QModelIndex& index, CSMWorld::ColumnBase::Display display) const;

        virtual void paint (QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const;
        ///< does nothing

        virtual QSize sizeHint (const QStyleOptionViewItem& option, const QModelIndex& index) const;
        ///< does nothing

    private slots:
        void editorDataCommited(QWidget* editor, const QModelIndex& index, CSMWorld::ColumnBase::Display display);

    };

    class DialogueSubView : public CSVDoc::SubView
    {
            QDataWidgetMapper *mWidgetMapper;
            DialogueDelegateDispatcher mDispatcher;

        public:

            DialogueSubView (const CSMWorld::UniversalId& id, CSMDoc::Document& document, bool createAndDelete = false);

            virtual void setEditLock (bool locked);
    };
}

#endif
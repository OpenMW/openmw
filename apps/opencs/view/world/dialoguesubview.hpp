#ifndef CSV_WORLD_DIALOGUESUBVIEW_H
#define CSV_WORLD_DIALOGUESUBVIEW_H

#include <map>
#include <memory>

#include <QAbstractItemDelegate>
#include <QScrollArea>

#include "../doc/subview.hpp"

#include "../../model/world/columnbase.hpp"
#include "../../model/world/commanddispatcher.hpp"

class QDataWidgetMapper;
class QSize;
class QEvent;
class QLabel;
class QVBoxLayout;

namespace CSMWorld
{
    class IdTable;
    class NestedTableProxyModel;
}

namespace CSMDoc
{
    class Document;
}

namespace CSVWorld
{
    class CommandDelegate;
    class CreatorFactoryBase;
    class TableBottomBox;

    class NotEditableSubDelegate : public QAbstractItemDelegate
    {
        const CSMWorld::IdTable* mTable;
    public:
        NotEditableSubDelegate(const CSMWorld::IdTable* table,
                               QObject * parent = 0);

        virtual void setEditorData (QWidget* editor, const QModelIndex& index) const;

        virtual void setModelData (QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const;

        virtual void paint (QPainter* painter,
                            const QStyleOptionViewItem& option,
                            const QModelIndex& index) const;
        ///< does nothing

        virtual QSize sizeHint (const QStyleOptionViewItem& option,
                                const QModelIndex& index) const;
        ///< does nothing

        virtual QWidget *createEditor (QWidget *parent,
                                const QStyleOptionViewItem& option,
                                const QModelIndex& index) const;
    };

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
        DialogueDelegateDispatcherProxy(QWidget* editor,
                                        CSMWorld::ColumnBase::Display display);
        QWidget* getEditor() const;

    public slots:
        void editorDataCommited();
        void setIndex(const QModelIndex& index);
        void tableMimeDataDropped(const std::vector<CSMWorld::UniversalId>& data,
                                  const CSMDoc::Document* document);

    signals:
        void editorDataCommited(QWidget* editor,
                                const QModelIndex& index,
                                CSMWorld::ColumnBase::Display display);

        void tableMimeDataDropped(QWidget* editor, const QModelIndex& index,
                                  const CSMWorld::UniversalId& id,
                                  const CSMDoc::Document* document);

    };

    class DialogueDelegateDispatcher : public QAbstractItemDelegate
    {
        Q_OBJECT
        std::map<int, CommandDelegate*> mDelegates;

        QObject* mParent;

        QAbstractItemModel* mTable;

        CSMWorld::CommandDispatcher& mCommandDispatcher;
        CSMDoc::Document& mDocument;

        NotEditableSubDelegate mNotEditableDelegate;

        std::vector<DialogueDelegateDispatcherProxy*> mProxys;
        //once we move to the C++11 we should use unique_ptr

    public:
        DialogueDelegateDispatcher(QObject* parent,
                                   CSMWorld::IdTable* table,
                                   CSMWorld::CommandDispatcher& commandDispatcher,
                                   CSMDoc::Document& document,
                                   QAbstractItemModel* model = 0);

        ~DialogueDelegateDispatcher();

        CSVWorld::CommandDelegate* makeDelegate(CSMWorld::ColumnBase::Display display);

        QWidget* makeEditor(CSMWorld::ColumnBase::Display display, const QModelIndex& index);
        ///< will return null if delegate is not present, parent of the widget is
        //same as for dispatcher itself

        virtual void setEditorData (QWidget* editor, const QModelIndex& index) const;

        virtual void setModelData (QWidget* editor, QAbstractItemModel* model,
                                   const QModelIndex& index) const;

        virtual void setModelData (QWidget* editor,
                                   QAbstractItemModel* model, const QModelIndex& index,
                                   CSMWorld::ColumnBase::Display display) const;

        virtual void paint (QPainter* painter,
                            const QStyleOptionViewItem& option,
                            const QModelIndex& index) const;
        ///< does nothing

        virtual QSize sizeHint (const QStyleOptionViewItem& option,
                                const QModelIndex& index) const;
        ///< does nothing

    private slots:
        void editorDataCommited(QWidget* editor, const QModelIndex& index,
                                CSMWorld::ColumnBase::Display display);

    signals:
        void tableMimeDataDropped(QWidget* editor, const QModelIndex& index,
                                  const CSMWorld::UniversalId& id,
                                  const CSMDoc::Document* document);
    };

    class EditWidget : public QScrollArea
    {
        Q_OBJECT
            QDataWidgetMapper *mWidgetMapper;
            QDataWidgetMapper *mNestedTableMapper;
            DialogueDelegateDispatcher mDispatcher;
            DialogueDelegateDispatcher *mNestedTableDispatcher;
            QWidget* mMainWidget;
            CSMWorld::IdTable* mTable;
            CSMWorld::CommandDispatcher& mCommandDispatcher;
            CSMDoc::Document& mDocument;
            std::vector<CSMWorld::NestedTableProxyModel*> mNestedModels; //Plain, raw C pointers, deleted in the dtor

        public:

            EditWidget (QWidget *parent, int row, CSMWorld::IdTable* table,
                        CSMWorld::CommandDispatcher& commandDispatcher,
                        CSMDoc::Document& document, bool createAndDelete = false);

            virtual ~EditWidget();

            void remake(int row);

        signals:
            void tableMimeDataDropped(QWidget* editor, const QModelIndex& index,
                                      const CSMWorld::UniversalId& id,
                                      const CSMDoc::Document* document);
    };

    class DialogueSubView : public CSVDoc::SubView
    {
        Q_OBJECT

        EditWidget* mEditWidget;
        QVBoxLayout* mMainLayout;
        CSMWorld::IdTable* mTable;
        QUndoStack& mUndoStack;
        std::string mCurrentId;
        bool mLocked;
        const CSMDoc::Document& mDocument;
        TableBottomBox* mBottom;
        CSMWorld::CommandDispatcher mCommandDispatcher;

        public:

            DialogueSubView (const CSMWorld::UniversalId& id,
                             CSMDoc::Document& document,
                             const CreatorFactoryBase& creatorFactory,
                             bool sorting = false);

            virtual void setEditLock (bool locked);

        private:
        void changeCurrentId(const std::string& newCurrent);

        private slots:

            void nextId();

            void prevId();

            void showPreview();

            void viewRecord();

            void cloneRequest();

            void dataChanged(const QModelIndex & index);
            ///\brief we need to care for deleting currently edited record

            void tableMimeDataDropped(QWidget* editor, const QModelIndex& index,
                                      const CSMWorld::UniversalId& id,
                                      const CSMDoc::Document* document);

            void requestFocus (const std::string& id);
    };
}

#endif

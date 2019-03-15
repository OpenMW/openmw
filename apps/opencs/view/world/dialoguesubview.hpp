#ifndef CSV_WORLD_DIALOGUESUBVIEW_H
#define CSV_WORLD_DIALOGUESUBVIEW_H

#include <set>
#include <map>
#include <memory>

#include <QAbstractItemDelegate>
#include <QScrollArea>

#ifndef Q_MOC_RUN
#include "../doc/subview.hpp"

#include "../../model/world/columnbase.hpp"
#include "../../model/world/commanddispatcher.hpp"
#include "../../model/world/universalid.hpp"
#endif

class QDataWidgetMapper;
class QSize;
class QEvent;
class QLabel;
class QVBoxLayout;
class QMenu;

namespace CSMWorld
{
    class IdTable;
    class NestedTableProxyModel;
}

namespace CSMPrefs
{
    class Setting;
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

        std::unique_ptr<refWrapper> mIndexWrapper;

    public:
        DialogueDelegateDispatcherProxy(QWidget* editor,
                                        CSMWorld::ColumnBase::Display display);
        QWidget* getEditor() const;

    public slots:
        void editorDataCommited();
        void setIndex(const QModelIndex& index);

    signals:
        void editorDataCommited(QWidget* editor,
                                const QModelIndex& index,
                                CSMWorld::ColumnBase::Display display);

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
    };

    /// A context menu with "Edit 'ID'" action for editors in the dialogue subview
    class IdContextMenu : public QObject
    {
            Q_OBJECT

            QWidget *mWidget;
            CSMWorld::UniversalId::Type mIdType;
            std::set<std::string> mExcludedIds;
            ///< A list of IDs that should not have the Edit 'ID' action.

            QMenu *mContextMenu;
            QAction *mEditIdAction;

            QString getWidgetValue() const;
            void addEditIdActionToMenu(const QString &text);
            void removeEditIdActionFromMenu();

        public:
            IdContextMenu(QWidget *widget, CSMWorld::ColumnBase::Display display);

            void excludeId(const std::string &id);

        private slots:
            void showContextMenu(const QPoint &pos);
            void editIdRequest();

        signals:
            void editIdRequest(const CSMWorld::UniversalId &id, const std::string &hint);
    };

    class EditWidget : public QScrollArea
    {
        Q_OBJECT
            QDataWidgetMapper *mWidgetMapper;
            QDataWidgetMapper *mNestedTableMapper;
            DialogueDelegateDispatcher *mDispatcher;
            DialogueDelegateDispatcher *mNestedTableDispatcher;
            QWidget* mMainWidget;
            CSMWorld::IdTable* mTable;
            CSMWorld::CommandDispatcher& mCommandDispatcher;
            CSMDoc::Document& mDocument;
            std::vector<CSMWorld::NestedTableProxyModel*> mNestedModels; //Plain, raw C pointers, deleted in the dtor

            void createEditorContextMenu(QWidget *editor,
                                         CSMWorld::ColumnBase::Display display,
                                         int currentRow) const;
        public:

            EditWidget (QWidget *parent, int row, CSMWorld::IdTable* table,
                        CSMWorld::CommandDispatcher& commandDispatcher,
                        CSMDoc::Document& document, bool createAndDelete = false);

            virtual ~EditWidget();

            void remake(int row);

        signals:
            void editIdRequest(const CSMWorld::UniversalId &id, const std::string &hint);
    };

    class SimpleDialogueSubView : public CSVDoc::SubView
    {
            Q_OBJECT

            EditWidget* mEditWidget;
            QVBoxLayout* mMainLayout;
            CSMWorld::IdTable* mTable;
            bool mLocked;
            const CSMDoc::Document& mDocument;
            CSMWorld::CommandDispatcher mCommandDispatcher;

        protected:

            QVBoxLayout& getMainLayout();

            CSMWorld::IdTable& getTable();

            CSMWorld::CommandDispatcher& getCommandDispatcher();

            EditWidget& getEditWidget();

            void updateCurrentId();

            bool isLocked() const;

        public:

            SimpleDialogueSubView (const CSMWorld::UniversalId& id, CSMDoc::Document& document);

            virtual void setEditLock (bool locked);

        private slots:

            void dataChanged(const QModelIndex & index);
            ///\brief we need to care for deleting currently edited record

            void rowsAboutToBeRemoved(const QModelIndex &parent, int start, int end);
    };

    class RecordButtonBar;

    class DialogueSubView : public SimpleDialogueSubView
    {
            Q_OBJECT

            TableBottomBox* mBottom;
            RecordButtonBar *mButtons;

        private:

            void addButtonBar();

        public:

            DialogueSubView (const CSMWorld::UniversalId& id, CSMDoc::Document& document,
                const CreatorFactoryBase& creatorFactory, bool sorting = false);

            virtual void setEditLock (bool locked);

        private slots:

            void settingChanged (const CSMPrefs::Setting *setting);

            void showPreview();

            void viewRecord();

            void switchToRow (int row);

            void requestFocus (const std::string& id);
    };
}

#endif

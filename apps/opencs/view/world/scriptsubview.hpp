#ifndef CSV_WORLD_SCRIPTSUBVIEW_H
#define CSV_WORLD_SCRIPTSUBVIEW_H

#include <QVBoxLayout>

#include "../../model/world/commanddispatcher.hpp"

#include "../doc/subview.hpp"

class QModelIndex;
class QLabel;
class QVBoxLayout;
class QSplitter;
class QTime;

namespace CSMDoc
{
    class Document;
}

namespace CSMWorld
{
    class IdTable;
}

namespace CSVWorld
{
    class ScriptEdit;
    class RecordButtonBar;
    class TableBottomBox;
    class ScriptErrorTable;

    class ScriptSubView : public CSVDoc::SubView
    {
            Q_OBJECT

            ScriptEdit *mEditor;
            CSMDoc::Document& mDocument;
            CSMWorld::IdTable *mModel;
            int mColumn;
            int mIdColumn;
            int mStateColumn;
            TableBottomBox *mBottom;
            RecordButtonBar *mButtons;
            CSMWorld::CommandDispatcher mCommandDispatcher;
            QVBoxLayout mLayout;
            QSplitter *mMain;
            ScriptErrorTable *mErrors;
            QTimer *mCompileDelay;

        private:

            void addButtonBar();

            void recompile();

            bool isDeleted() const;

            void updateDeletedState();

        public:

            ScriptSubView (const CSMWorld::UniversalId& id, CSMDoc::Document& document);

            virtual void setEditLock (bool locked);

            virtual void useHint (const std::string& hint);

            virtual void updateUserSetting (const QString& name, const QStringList& value);

            virtual void setStatusBar (bool show);

        public slots:

            void textChanged();

            void dataChanged (const QModelIndex& topLeft, const QModelIndex& bottomRight);

            void rowsAboutToBeRemoved (const QModelIndex& parent, int start, int end);

        private slots:

            void updateStatusBar();

            void switchToRow (int row);

            void switchToId (const std::string& id);

            void highlightError (int line, int column);

            void updateRequest();
    };
}

#endif

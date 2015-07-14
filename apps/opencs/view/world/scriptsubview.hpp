#ifndef CSV_WORLD_SCRIPTSUBVIEW_H
#define CSV_WORLD_SCRIPTSUBVIEW_H

#include <QVBoxLayout>

#include "../../model/world/commanddispatcher.hpp"

#include "../doc/subview.hpp"

class QModelIndex;
class QLabel;
class QVBoxLayout;

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

    class ScriptSubView : public CSVDoc::SubView
    {
            Q_OBJECT

            ScriptEdit *mEditor;
            CSMDoc::Document& mDocument;
            CSMWorld::IdTable *mModel;
            int mColumn;
            TableBottomBox *mBottom;
            RecordButtonBar *mButtons;
            CSMWorld::CommandDispatcher mCommandDispatcher;
            QVBoxLayout mLayout;

        private:

            void addButtonBar();

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
    };
}

#endif

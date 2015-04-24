#ifndef CSV_WORLD_SCRIPTSUBVIEW_H
#define CSV_WORLD_SCRIPTSUBVIEW_H

#include "../doc/subview.hpp"

class QModelIndex;

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

    class ScriptSubView : public CSVDoc::SubView
    {
            Q_OBJECT

            ScriptEdit *mEditor;
            CSMDoc::Document& mDocument;
            CSMWorld::IdTable *mModel;
            int mColumn;

        public:

            ScriptSubView (const CSMWorld::UniversalId& id, CSMDoc::Document& document);

            virtual void setEditLock (bool locked);

            virtual void useHint (const std::string& hint);

        public slots:

            void textChanged();

            void dataChanged (const QModelIndex& topLeft, const QModelIndex& bottomRight);

            void rowsAboutToBeRemoved (const QModelIndex& parent, int start, int end);
    };
}

#endif

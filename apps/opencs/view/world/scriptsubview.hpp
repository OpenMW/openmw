#ifndef CSV_WORLD_SCRIPTSUBVIEW_H
#define CSV_WORLD_SCRIPTSUBVIEW_H

#include "../doc/subview.hpp"

#include <QTimer>

class QTextEdit;
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
    class ScriptHighlighter;

    class ScriptSubView : public CSVDoc::SubView
    {
            Q_OBJECT

            QTextEdit *mEditor;
            CSMDoc::Document& mDocument;
            CSMWorld::IdTable *mModel;
            int mColumn;
            int mChangeLocked;
            ScriptHighlighter *mHighlighter;
            QTimer mUpdateTimer;

            class ChangeLock
            {
                    ScriptSubView& mView;

                    ChangeLock (const ChangeLock&);
                    ChangeLock& operator= (const ChangeLock&);

                public:

                    ChangeLock (ScriptSubView& view);
                    ~ChangeLock();
            };

            friend class ChangeLock;

        public:

            ScriptSubView (const CSMWorld::UniversalId& id, CSMDoc::Document& document);

            virtual void setEditLock (bool locked);

        public slots:

            void idListChanged();

            void textChanged();

            void dataChanged (const QModelIndex& topLeft, const QModelIndex& bottomRight);

            void rowsAboutToBeRemoved (const QModelIndex& parent, int start, int end);

        private slots:

            void updateHighlighting();
    };
}

#endif
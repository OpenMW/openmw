#ifndef CSV_TOOLS_SEARCHSUBVIEW_H
#define CSV_TOOLS_SEARCHSUBVIEW_H

#include "../../model/tools/search.hpp"

#include "../doc/subview.hpp"

#include "searchbox.hpp"

class QTableView;
class QModelIndex;

namespace CSMDoc
{
    class Document;
}

namespace CSVWorld
{
    class TableBottomBox;
}

namespace CSVTools
{
    class ReportTable;

    class SearchSubView : public CSVDoc::SubView
    {
            Q_OBJECT

            ReportTable *mTable;
            SearchBox mSearchBox;
            CSMDoc::Document& mDocument;
            CSMTools::Search mSearch;
            bool mLocked;
            CSVWorld::TableBottomBox *mBottom;

        private:

            void replace (bool selection);

        protected:

            void showEvent (QShowEvent *event);

        public:

            SearchSubView (const CSMWorld::UniversalId& id, CSMDoc::Document& document);

            virtual void setEditLock (bool locked);

            virtual void setStatusBar (bool show);

        private slots:

            void stateChanged (int state, CSMDoc::Document *document);

            void startSearch (const CSMTools::Search& search);

            void replaceRequest();

            void replaceAllRequest();

            void tableSizeUpdate();

            void operationDone (int type, bool failed);
    };
}

#endif

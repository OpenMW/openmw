#ifndef CSV_TOOLS_REPORTSUBVIEW_H
#define CSV_TOOLS_REPORTSUBVIEW_H

#include "../doc/subview.hpp"

class QTableView;
class QModelIndex;

namespace CSMDoc
{
    class Document;
}

namespace CSVTools
{
    class ReportTable;

    class ReportSubView : public CSVDoc::SubView
    {
            Q_OBJECT

            ReportTable *mTable;
            CSMDoc::Document& mDocument;
            int mRefreshState;

        public:

            ReportSubView (const CSMWorld::UniversalId& id, CSMDoc::Document& document);

            void setEditLock (bool locked) override;

        private slots:

            void refreshRequest();
    };
}

#endif

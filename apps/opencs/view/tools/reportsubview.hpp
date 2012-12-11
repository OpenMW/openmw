#ifndef CSV_TOOLS_REPORTSUBVIEW_H
#define CSV_TOOLS_REPORTSUBVIEW_H

#include "../doc/subview.hpp"

class QTableView;

namespace CSMDoc
{
    class Document;
}

namespace CSVTools
{
    class Table;

    class ReportSubView : public CSVDoc::SubView
    {
            QTableView *mTable;

        public:

            ReportSubView (const CSMWorld::UniversalId& id, CSMDoc::Document& document);

            virtual void setEditLock (bool locked);
    };
}

#endif
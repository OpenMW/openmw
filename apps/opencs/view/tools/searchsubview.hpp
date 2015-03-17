#ifndef CSV_TOOLS_SEARCHSUBVIEW_H
#define CSV_TOOLS_SEARCHSUBVIEW_H

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

    class SearchSubView : public CSVDoc::SubView
    {
            Q_OBJECT

            ReportTable *mTable;

        public:

            SearchSubView (const CSMWorld::UniversalId& id, CSMDoc::Document& document);

            virtual void setEditLock (bool locked);

            virtual void updateUserSetting (const QString &, const QStringList &);
    };
}

#endif

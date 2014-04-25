#ifndef CSV_TOOLS_REPORTSUBVIEW_H
#define CSV_TOOLS_REPORTSUBVIEW_H

#include "../doc/subview.hpp"

class QTableView;
class QModelIndex;

namespace CSMDoc
{
    class Document;
}

namespace CSMTools
{
    class ReportModel;
}

namespace CSVWorld
{
    class CommandDelegate;
}

namespace CSVTools
{
    class Table;

    class ReportSubView : public CSVDoc::SubView
    {
            Q_OBJECT

            CSMTools::ReportModel *mModel;
            QTableView *mTable;
            CSVWorld::CommandDelegate *mIdTypeDelegate;

        public:

            ReportSubView (const CSMWorld::UniversalId& id, CSMDoc::Document& document);

            virtual void setEditLock (bool locked);

            virtual void updateUserSetting
                                        (const QString &, const QStringList &);

        private slots:

            void show (const QModelIndex& index);
    };
}

#endif

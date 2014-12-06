#ifndef CSV_TOOLS_REPORTTABLE_H
#define CSV_TOOLS_REPORTTABLE_H

#include "../world/dragrecordtable.hpp"

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
    class ReportTable : public CSVWorld::DragRecordTable
    {
            Q_OBJECT

            CSMTools::ReportModel *mModel;
            CSVWorld::CommandDelegate *mIdTypeDelegate;

        private:

            void mouseMoveEvent (QMouseEvent *event);

        public:

            ReportTable (CSMDoc::Document& document, const CSMWorld::UniversalId& id,
                QWidget *parent = 0);

            virtual std::vector<CSMWorld::UniversalId> getDraggedRecords() const;

            void updateUserSetting (const QString& name, const QStringList& list);

        private slots:

            void show (const QModelIndex& index);

        signals:

            void editRequest (const CSMWorld::UniversalId& id, const std::string& hint);
    };
}

#endif

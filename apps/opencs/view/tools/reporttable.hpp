#ifndef CSV_TOOLS_REPORTTABLE_H
#define CSV_TOOLS_REPORTTABLE_H

#include "../world/dragrecordtable.hpp"

class QAction;

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
            QAction *mShowAction;
            QAction *mRemoveAction;

        private:

            void contextMenuEvent (QContextMenuEvent *event);

            void mouseMoveEvent (QMouseEvent *event);

        public:

            ReportTable (CSMDoc::Document& document, const CSMWorld::UniversalId& id,
                QWidget *parent = 0);

            virtual std::vector<CSMWorld::UniversalId> getDraggedRecords() const;

            void updateUserSetting (const QString& name, const QStringList& list);

        private slots:

            void show (const QModelIndex& index);

            void showSelection();

            void removeSelection();

        signals:

            void editRequest (const CSMWorld::UniversalId& id, const std::string& hint);
    };
}

#endif

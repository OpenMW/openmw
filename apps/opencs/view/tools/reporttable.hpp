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
            QAction *mReplaceAction;

        private:

            void contextMenuEvent (QContextMenuEvent *event);

            void mouseMoveEvent (QMouseEvent *event);

            virtual void mouseDoubleClickEvent (QMouseEvent *event);

        public:

            /// \param richTextDescription Use rich text in the description column.
            ReportTable (CSMDoc::Document& document, const CSMWorld::UniversalId& id,
                bool richTextDescription, QWidget *parent = 0);

            virtual std::vector<CSMWorld::UniversalId> getDraggedRecords() const;

            void updateUserSetting (const QString& name, const QStringList& list);

            void clear();

            // Return indices of rows that are suitable for replacement.
            //
            // \param selection Only list selected rows.
            std::vector<int> getReplaceIndices (bool selection) const;

            void flagAsReplaced (int index);

        private slots:

            void showSelection();

            void removeSelection();

        signals:

            void editRequest (const CSMWorld::UniversalId& id, const std::string& hint);

            void replaceRequest();
    };
}

#endif

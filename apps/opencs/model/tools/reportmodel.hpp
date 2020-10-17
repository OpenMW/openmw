#ifndef CSM_TOOLS_REPORTMODEL_H
#define CSM_TOOLS_REPORTMODEL_H

#include <vector>
#include <string>

#include <QAbstractTableModel>

#include "../doc/messages.hpp"

#include "../world/universalid.hpp"

namespace CSMTools
{
    class ReportModel : public QAbstractTableModel
    {
            Q_OBJECT

            std::vector<CSMDoc::Message> mRows;

            // Fixed columns
            enum Columns
            {
                Column_Type = 0, Column_Id = 1, Column_Hint = 2
            };

            // Configurable columns
            int mColumnDescription;
            int mColumnField;
            int mColumnSeverity;

        public:

            ReportModel (bool fieldColumn = false, bool severityColumn = true);
        
            int rowCount (const QModelIndex & parent = QModelIndex()) const override;

            int columnCount (const QModelIndex & parent = QModelIndex()) const override;

            QVariant data (const QModelIndex & index, int role = Qt::DisplayRole) const override;

            QVariant headerData (int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

            bool removeRows (int row, int count, const QModelIndex& parent = QModelIndex()) override;
            
            void add (const CSMDoc::Message& message);

            void flagAsReplaced (int index);
                
            const CSMWorld::UniversalId& getUniversalId (int row) const;

            std::string getHint (int row) const;

            void clear();

            // Return number of messages with Error or SeriousError severity.
            int countErrors() const;
    };
}

#endif

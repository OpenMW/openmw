#ifndef CSM_TOOLS_REPORTMODEL_H
#define CSM_TOOLS_REPORTMODEL_H

#include <vector>
#include <string>

#include <QAbstractTableModel>

#include "../world/universalid.hpp"

namespace CSMTools
{
    class ReportModel : public QAbstractTableModel
    {
            Q_OBJECT

            struct Line
            {
                Line (const CSMWorld::UniversalId& id, const std::string& message,
                    const std::string& hint);
                
                CSMWorld::UniversalId mId;
                std::string mMessage;
                std::string mHint;
            };

            std::vector<Line> mRows;

            // Fixed columns
            enum Columns
            {
                Column_Type = 0, Column_Id = 1, Column_Hint = 2
            };

            // Configurable columns
            int mColumnDescription;
            int mColumnField;

        public:

            ReportModel (bool fieldColumn = false);
        
            virtual int rowCount (const QModelIndex & parent = QModelIndex()) const;

            virtual int columnCount (const QModelIndex & parent = QModelIndex()) const;

            virtual QVariant data (const QModelIndex & index, int role = Qt::DisplayRole) const;

            virtual QVariant headerData (int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;

            virtual bool removeRows (int row, int count, const QModelIndex& parent = QModelIndex());
            
            void add (const CSMWorld::UniversalId& id, const std::string& message,
                const std::string& hint = "");

            void flagAsReplaced (int index);
                
            const CSMWorld::UniversalId& getUniversalId (int row) const;

            std::string getHint (int row) const;

            void clear();
    };
}

#endif

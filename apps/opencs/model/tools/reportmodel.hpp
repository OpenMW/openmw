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

            std::vector<std::pair<CSMWorld::UniversalId, std::string> > mRows;

        public:

            virtual int rowCount (const QModelIndex & parent = QModelIndex()) const;

            virtual int columnCount (const QModelIndex & parent = QModelIndex()) const;

            virtual QVariant data (const QModelIndex & index, int role = Qt::DisplayRole) const;

            virtual QVariant headerData (int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;

            virtual bool removeRows (int row, int count, const QModelIndex& parent = QModelIndex());

            void add (const std::string& row);

            const CSMWorld::UniversalId& getUniversalId (int row) const;
    };
}

#endif

#ifndef CSVWORLD_TABLEEDITIDACTION_HPP
#define CSVWORLD_TABLEEDITIDACTION_HPP

#include <QAction>

#include "../../model/world/columnbase.hpp"
#include "../../model/world/universalid.hpp"

class QTableView;

namespace CSVWorld
{
    class TableEditIdAction : public QAction
    {
            const QTableView &mTable;
            CSMWorld::UniversalId mCurrentId;

            typedef std::pair<CSMWorld::ColumnBase::Display, QString> CellData;
            CellData getCellData(int row, int column) const;

        public:
            TableEditIdAction(const QTableView &table, QWidget *parent = nullptr);

            void setCell(int row, int column);

            CSMWorld::UniversalId getCurrentId() const;
            bool isValidIdCell(int row, int column) const;
    };
}

#endif

#include "tableeditidaction.hpp"

#include <QTableView>

#include "../../model/world/tablemimedata.hpp"

CSVWorld::TableEditIdAction::CellData CSVWorld::TableEditIdAction::getCellData(int row, int column) const
{
    QModelIndex index = mTable.model()->index(row, column);
    if (index.isValid())
    {
        QVariant display = mTable.model()->data(index, CSMWorld::ColumnBase::Role_Display);
        QString value = mTable.model()->data(index).toString();
        return std::make_pair(static_cast<CSMWorld::ColumnBase::Display>(display.toInt()), value);
    }
    return std::make_pair(CSMWorld::ColumnBase::Display_None, "");
}

CSVWorld::TableEditIdAction::TableEditIdAction(const QTableView &table, QWidget *parent)
    : QAction(parent),
      mTable(table),
      mCurrentId(CSMWorld::UniversalId::Type_None)
{}

void CSVWorld::TableEditIdAction::setCell(int row, int column)
{
    CellData data = getCellData(row, column);
    CSMWorld::UniversalId::Type idType = CSMWorld::TableMimeData::convertEnums(data.first);

    if (idType != CSMWorld::UniversalId::Type_None)
    {
        mCurrentId = CSMWorld::UniversalId(idType, data.second.toUtf8().constData());
        setText("Edit '" + data.second + "'");
    }
}

CSMWorld::UniversalId CSVWorld::TableEditIdAction::getCurrentId() const
{
    return mCurrentId;
}

bool CSVWorld::TableEditIdAction::isValidIdCell(int row, int column) const
{
    CellData data = getCellData(row, column);
    CSMWorld::UniversalId::Type idType = CSMWorld::TableMimeData::convertEnums(data.first);
    return CSMWorld::ColumnBase::isId(data.first) && 
           idType != CSMWorld::UniversalId::Type_None &&
           !data.second.isEmpty();
}

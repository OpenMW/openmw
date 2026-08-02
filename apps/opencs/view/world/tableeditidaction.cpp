#include "tableeditidaction.hpp"

#include <QTableView>

#include <components/esm/refid.hpp>

#include "../../model/world/data.hpp"
#include "../../model/world/idtable.hpp"
#include "../../model/world/tablemimedata.hpp"

CSVWorld::TableEditIdAction::TableEditIdAction(const QTableView& table, CSMWorld::Data& data, QWidget* parent)
    : QAction(parent)
    , mTable(table)
    , mData(data)
    , mCurrentId(CSMWorld::UniversalId::Type_None)
{
}

bool CSVWorld::TableEditIdAction::setCell(int row, int column)
{
    QModelIndex index = mTable.model()->index(row, column);
    if (!index.isValid())
        return false;

    QVariant displayVar = index.data(CSMWorld::ColumnBase::Role_Display);
    const auto display = static_cast<CSMWorld::ColumnBase::Display>(displayVar.toInt());
    const QString name = index.data(Qt::DisplayRole).toString();

    if (!CSMWorld::ColumnBase::isId(display) || name.isEmpty())
        return false;

    CSMWorld::UniversalId::Type idType = CSMWorld::TableMimeData::convertEnums(display);
    if (idType == CSMWorld::UniversalId::Type_None)
        return false;

    mCurrentId = CSMWorld::UniversalId(idType, name.toStdString());
    if (mCurrentId.getClass() == CSMWorld::UniversalId::Class_Resource)
        return false;

    if (mCurrentId.getClass() == CSMWorld::UniversalId::Class_RefRecord)
        idType = CSMWorld::UniversalId::Type_Referenceable;

    CSMWorld::IdTable* idModel = dynamic_cast<CSMWorld::IdTable*>(mData.getTableModel(idType));
    if (idModel && !idModel->getModelIndex(mCurrentId.getId(), 0).isValid())
        return false;

    setText("Edit '" + name + "'");
    return true;
}

CSMWorld::UniversalId CSVWorld::TableEditIdAction::getCurrentId() const
{
    return mCurrentId;
}

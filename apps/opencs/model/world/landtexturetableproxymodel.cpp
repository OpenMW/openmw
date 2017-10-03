#include "landtexturetableproxymodel.hpp"

#include "idtable.hpp"

namespace CSMWorld
{
    LandTextureTableProxyModel::LandTextureTableProxyModel(QObject* parent)
        : IdTableProxyModel(parent)
    {
    }

    bool LandTextureTableProxyModel::filterAcceptsRow (int sourceRow, const QModelIndex& sourceParent) const
    {
        int columnIndex = mSourceModel->findColumnIndex(Columns::ColumnId_Modification);
        QModelIndex index = mSourceModel->index(sourceRow, columnIndex);
        if (mSourceModel->data(index).toInt() != RecordBase::State_ModifiedOnly)
            return false;

        return IdTableProxyModel::filterAcceptsRow(sourceRow, sourceParent);
    }
}

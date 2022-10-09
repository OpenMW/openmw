#include "landtexturetableproxymodel.hpp"

namespace CSMWorld
{
    LandTextureTableProxyModel::LandTextureTableProxyModel(QObject* parent)
        : IdTableProxyModel(parent)
    {
    }

    bool LandTextureTableProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const
    {
        return IdTableProxyModel::filterAcceptsRow(sourceRow, sourceParent);
    }
}

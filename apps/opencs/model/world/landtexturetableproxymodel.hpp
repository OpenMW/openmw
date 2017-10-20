#ifndef CSM_WORLD_LANDTEXTURETABLEPROXYMODEL_H
#define CSM_WORLD_LANDTEXTURETABLEPROXYMODEL_H

#include "idtableproxymodel.hpp"

namespace CSMWorld
{
    /// \brief Removes base records from filtered results.
    class LandTextureTableProxyModel : public IdTableProxyModel
    {
            Q_OBJECT
        public:

            LandTextureTableProxyModel(QObject* parent = nullptr);

        protected:

            bool filterAcceptsRow (int sourceRow, const QModelIndex& sourceParent) const override;
    };
}

#endif

#ifndef CSM_FILTER_ORNODE_H
#define CSM_FILTER_ORNODE_H

#include "narynode.hpp"

#include <map>
#include <memory>
#include <vector>

namespace CSMWorld
{
    class IdTableBase;
}

namespace CSMFilter
{
    class Node;
    class OrNode : public NAryNode
    {
    public:
        OrNode(const std::vector<std::shared_ptr<Node>>& nodes);

        bool test(const CSMWorld::IdTableBase& table, int row, const std::map<int, int>& columns) const override;
        ///< \return Can the specified table row pass through to filter?
        /// \param columns column ID to column index mapping
    };
}

#endif

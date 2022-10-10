#ifndef CSM_FILTER_ANDNODE_H
#define CSM_FILTER_ANDNODE_H

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
    class AndNode : public NAryNode
    {
    public:
        AndNode(const std::vector<std::shared_ptr<Node>>& nodes);

        bool test(const CSMWorld::IdTableBase& table, int row, const std::map<int, int>& columns) const override;
        ///< \return Can the specified table row pass through to filter?
        /// \param columns column ID to column index mapping
    };
}

#endif

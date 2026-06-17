#ifndef CSM_FILTER_LEAFNODE_H
#define CSM_FILTER_LEAFNODE_H

#include <vector>

#include "node.hpp"

namespace CSMFilter
{
    class LeafNode : public Node
    {
    public:
        std::vector<int> getReferencedColumns() const override { return {}; }
        ///< Return a list of the IDs of the columns referenced by this node. The column mapping
        /// passed into test as columns must contain all columns listed here.
    };
}

#endif

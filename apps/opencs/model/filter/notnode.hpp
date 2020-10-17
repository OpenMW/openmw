#ifndef CSM_FILTER_NOTNODE_H
#define CSM_FILTER_NOTNODE_H

#include "unarynode.hpp"

namespace CSMFilter
{
    class NotNode : public UnaryNode
    {
        public:

            NotNode (std::shared_ptr<Node> child);

            bool test (const CSMWorld::IdTableBase& table, int row,
                const std::map<int, int>& columns) const override;
            ///< \return Can the specified table row pass through to filter?
            /// \param columns column ID to column index mapping
    };
}

#endif

#ifndef CSM_FILTER_ORNODE_H
#define CSM_FILTER_ORNODE_H

#include "narynode.hpp"

namespace CSMFilter
{
    class OrNode : public NAryNode
    {
        public:

            OrNode (const std::vector<std::shared_ptr<Node> >& nodes);

            bool test (const CSMWorld::IdTableBase& table, int row,
                const std::map<int, int>& columns) const override;
            ///< \return Can the specified table row pass through to filter?
            /// \param columns column ID to column index mapping
    };
}

#endif

#ifndef CSM_FILTER_ORNODE_H
#define CSM_FILTER_ORNODE_H

#include "narynode.hpp"

namespace CSMFilter
{
    class OrNode : public NAryNode
    {
            bool mAnd;

        public:

            OrNode (const std::vector<boost::shared_ptr<Node> >& nodes);

            virtual bool test (const CSMWorld::IdTable& table, int row,
                const std::map<int, int>& columns) const;
            ///< \return Can the specified table row pass through to filter?
            /// \param columns column ID to column index mapping
    };
}

#endif

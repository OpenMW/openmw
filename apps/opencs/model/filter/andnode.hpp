#ifndef CSM_FILTER_ANDNODE_H
#define CSM_FILTER_ANDNODE_H

#include "narynode.hpp"

namespace CSMFilter
{
    class AndNode : public NAryNode
    {
            bool mAnd;

        public:

            AndNode (const std::vector<boost::shared_ptr<Node> >& nodes);

            virtual bool test (const CSMWorld::IdTable& table, int row,
                const std::map<int, int>& columns) const;
            ///< \return Can the specified table row pass through to filter?
            /// \param columns column ID to column index mapping
    };
}

#endif

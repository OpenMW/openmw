#ifndef CSM_FILTER_BOOLEANNODE_H
#define CSM_FILTER_BOOLEANNODE_H

#include "leafnode.hpp"

namespace CSMFilter
{
    class BooleanNode : public LeafNode
    {
            bool mTrue;

        public:

            BooleanNode (bool true_);

            bool test (const CSMWorld::IdTableBase& table, int row,
                const std::map<int, int>& columns) const override;
            ///< \return Can the specified table row pass through to filter?
            /// \param columns column ID to column index mapping

            std::string toString (bool numericColumns) const override;
            ///< Return a string that represents this node.
            ///
            /// \param numericColumns Use numeric IDs instead of string to represent columns.

    };
}

#endif

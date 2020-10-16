#ifndef CSM_FILTER_TEXTNODE_H
#define CSM_FILTER_TEXTNODE_H

#include "leafnode.hpp"

namespace CSMFilter
{
    class TextNode : public LeafNode
    {
            int mColumnId;
            std::string mText;

        public:

            TextNode (int columnId, const std::string& text);

            bool test (const CSMWorld::IdTableBase& table, int row,
                const std::map<int, int>& columns) const override;
            ///< \return Can the specified table row pass through to filter?
            /// \param columns column ID to column index mapping

            std::vector<int> getReferencedColumns() const override;
            ///< Return a list of the IDs of the columns referenced by this node. The column mapping
            /// passed into test as columns must contain all columns listed here.

            std::string toString (bool numericColumns) const override;
            ///< Return a string that represents this node.
            ///
            /// \param numericColumns Use numeric IDs instead of string to represent columns.
    };
}

#endif

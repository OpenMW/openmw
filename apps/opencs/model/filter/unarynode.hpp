#ifndef CSM_FILTER_UNARYNODE_H
#define CSM_FILTER_UNARYNODE_H

#include "node.hpp"

namespace CSMFilter
{
    class UnaryNode : public Node
    {
            std::shared_ptr<Node> mChild;
            std::string mName;

        public:

            UnaryNode (std::shared_ptr<Node> child, const std::string& name);

            const Node& getChild() const;

            Node& getChild();

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

#ifndef CSM_FILTER_NARYNODE_H
#define CSM_FILTER_NARYNODE_H

#include <vector>
#include <string>

#include "node.hpp"

namespace CSMFilter
{
    class NAryNode : public Node
    {
            std::vector<std::shared_ptr<Node> > mNodes;
            std::string mName;

        public:

            NAryNode (const std::vector<std::shared_ptr<Node> >& nodes, const std::string& name);

            int getSize() const;

            const Node& operator[] (int index) const;

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

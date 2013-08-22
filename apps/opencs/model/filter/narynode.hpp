#ifndef CSM_FILTER_NARYNODE_H
#define CSM_FILTER_NARYNODE_H

#include <vector>

#include <boost/shared_ptr.hpp>

#include "node.hpp"

namespace CSMFilter
{
    class NAryNode : public Node
    {
            std::vector<boost::shared_ptr<Node> > mNodes;

        public:

            NAryNode (const std::vector<boost::shared_ptr<Node> >& nodes);

            int getSize() const;

            const Node& operator[] (int index) const;

            virtual std::vector<int> getReferencedColumns() const;
            ///< Return a list of the IDs of the columns referenced by this node. The column mapping
            /// passed into test as columns must contain all columns listed here.

            virtual bool isSimple() const;
            ///< \return Can this filter be displayed in simple mode.

    };
}

#endif

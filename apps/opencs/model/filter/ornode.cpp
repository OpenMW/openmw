#include "ornode.hpp"

#include <sstream>

CSMFilter::OrNode::OrNode (const std::vector<std::shared_ptr<Node> >& nodes)
: NAryNode (nodes, "or")
{}

bool CSMFilter::OrNode::test (const CSMWorld::IdTableBase& table, int row,
    const std::map<int, int>& columns) const
{
    int size = getSize();

    for (int i=0; i<size; ++i)
        if ((*this)[i].test (table, row, columns))
            return true;

    return false;
}

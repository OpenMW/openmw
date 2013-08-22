
#include "leafnode.hpp"

std::vector<int> CSMFilter::LeafNode::getReferencedColumns() const
{
    return std::vector<int>();
}

bool CSMFilter::LeafNode::isSimple() const
{
    return true;
}

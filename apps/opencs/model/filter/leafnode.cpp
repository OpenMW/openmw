
#include "leafnode.hpp"

std::vector<std::string> CSMFilter::LeafNode::getReferencedFilters() const
{
    return std::vector<std::string>();
}

std::vector<int> CSMFilter::LeafNode::getReferencedColumns() const
{
    return std::vector<int>();
}

bool CSMFilter::LeafNode::isSimple() const
{
    return true;
}

bool CSMFilter::LeafNode::hasUserValue() const
{
    return false;
}

#include "unarynode.hpp"

CSMFilter::UnaryNode::UnaryNode (std::auto_ptr<Node> child) : mChild (child) {}

const CSMFilter::Node& CSMFilter::UnaryNode::getChild() const
{
    return *mChild;
}

CSMFilter::Node& CSMFilter::UnaryNode::getChild()
{
    return *mChild;
}

std::vector<std::string> CSMFilter::UnaryNode::getReferencedFilters() const
{
    return mChild->getReferencedFilters();
}

std::vector<int> CSMFilter::UnaryNode::getReferencedColumns() const
{
    return mChild->getReferencedColumns();
}

bool CSMFilter::UnaryNode::isSimple() const
{
    return false;
}

bool CSMFilter::UnaryNode::hasUserValue() const
{
    return mChild->hasUserValue();
}

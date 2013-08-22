
#include "unarynode.hpp"

CSMFilter::UnaryNode::UnaryNode (boost::shared_ptr<Node> child) : mChild (child) {}

const CSMFilter::Node& CSMFilter::UnaryNode::getChild() const
{
    return *mChild;
}

CSMFilter::Node& CSMFilter::UnaryNode::getChild()
{
    return *mChild;
}

std::vector<int> CSMFilter::UnaryNode::getReferencedColumns() const
{
    return mChild->getReferencedColumns();
}

bool CSMFilter::UnaryNode::isSimple() const
{
    return false;
}


#include "unarynode.hpp"

CSMFilter::UnaryNode::UnaryNode (std::shared_ptr<Node> child, const std::string& name)
: mChild (child), mName (name)
{}

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

std::string CSMFilter::UnaryNode::toString (bool numericColumns) const
{
    return mName + " " + mChild->toString (numericColumns);
}

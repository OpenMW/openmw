
#include "binarynode.hpp"

CSMFilter::BinaryNode::BinaryNode (boost::shared_ptr<Node> left, boost::shared_ptr<Node> right)
: mLeft (left), mRight (right)
{}

const CSMFilter::Node& CSMFilter::BinaryNode::getLeft() const
{
    return *mLeft;
}

CSMFilter::Node& CSMFilter::BinaryNode::getLeft()
{
    return *mLeft;
}

const CSMFilter::Node& CSMFilter::BinaryNode::getRight() const
{
    return *mRight;
}

CSMFilter::Node& CSMFilter::BinaryNode::getRight()
{
    return *mRight;
}

std::vector<std::string> CSMFilter::BinaryNode::getReferencedFilters() const
{
    std::vector<std::string> left = mLeft->getReferencedFilters();

    std::vector<std::string> right = mRight->getReferencedFilters();

    left.insert (left.end(), right.begin(), right.end());

    return left;
}

std::vector<int> CSMFilter::BinaryNode::getReferencedColumns() const
{
    std::vector<int> left = mLeft->getReferencedColumns();

    std::vector<int> right = mRight->getReferencedColumns();

    left.insert (left.end(), right.begin(), right.end());

    return left;
}

bool CSMFilter::BinaryNode::isSimple() const
{
    return false;
}

bool CSMFilter::BinaryNode::hasUserValue() const
{
    return mLeft->hasUserValue() || mRight->hasUserValue();
}

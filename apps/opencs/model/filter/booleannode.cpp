#include "booleannode.hpp"

CSMFilter::BooleanNode::BooleanNode (bool true_) : mTrue (true_) {}

bool CSMFilter::BooleanNode::test (const CSMWorld::IdTableBase& table, int row,
    const std::map<int, int>& columns) const
{
    return mTrue;
}

std::string CSMFilter::BooleanNode::toString (bool numericColumns) const
{
    return mTrue ? "true" : "false";
}

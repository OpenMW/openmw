
#include "booleannode.hpp"

CSMFilter::BooleanNode::BooleanNode (bool true_) : mTrue (true_) {}

bool CSMFilter::BooleanNode::test (const CSMWorld::IdTable& table, int row,
    const std::map<std::string, const Node *>& otherFilters,
    const std::map<int, int>& columns,
    const std::string& userValue) const
{
    return mTrue;
}

std::string CSMFilter::BooleanNode::toString (bool numericColumns) const
{
    return mTrue ? "true" : "false";
}
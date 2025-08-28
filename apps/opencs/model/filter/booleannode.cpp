#include "booleannode.hpp"

namespace CSMWorld
{
    class IdTableBase;
}

CSMFilter::BooleanNode::BooleanNode(bool value)
    : mTrue(value)
{
}

bool CSMFilter::BooleanNode::test(const CSMWorld::IdTableBase& table, int row, const std::map<int, int>& columns) const
{
    return mTrue;
}

std::string CSMFilter::BooleanNode::toString(bool numericColumns) const
{
    return mTrue ? "true" : "false";
}

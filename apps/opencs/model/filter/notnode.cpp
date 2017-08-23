#include "notnode.hpp"

CSMFilter::NotNode::NotNode (std::shared_ptr<Node> child) : UnaryNode (child, "not") {}

bool CSMFilter::NotNode::test (const CSMWorld::IdTableBase& table, int row,
    const std::map<int, int>& columns) const
{
    return !getChild().test (table, row, columns);
}

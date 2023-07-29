#include "notnode.hpp"

#include <apps/opencs/model/filter/node.hpp>
#include <apps/opencs/model/filter/unarynode.hpp>

namespace CSMWorld
{
    class IdTableBase;
}

CSMFilter::NotNode::NotNode(std::shared_ptr<Node> child)
    : UnaryNode(std::move(child), "not")
{
}

bool CSMFilter::NotNode::test(const CSMWorld::IdTableBase& table, int row, const std::map<int, int>& columns) const
{
    return !getChild().test(table, row, columns);
}
